## c++20 协程

[C++20-Coroutines-cppreference](https://en.cppreference.com/w/cpp/language/coroutines)开篇就一句话总结了：协程是“可以暂停和恢复的函数”。这句话很好地概括了协程的核心概念。

以下是三个使用协程的场景：

```cpp
// A:
task<> tcp_echo_server()
{
    char data[1024];
    while (true)
    {
        std::size_t n = co_await socket.async_read_some(buffer(data));
        co_await async_write(socket, buffer(data, n));
    }
}

// B:
generator<unsigned int> iota(unsigned int n = 0)
{
    while (true)
        co_yield n++;
}

// C:
lazy<int> f()
{
    co_return 7;
}
```

### 1. 协程的原理

因为协程的目的是为了实现“可以暂停和恢复的”函数，为了实现这个功能，一个朴素的思想方式其实就是跨函数的`goto`。当前函数执行到一半，就可以使用`goto`跳转到另一个函数，然后再跳转回来。

```c++
void funA(){
    //...
    int a = 1;
    goto labelB
    //...
    goto labelC
}

void funB(){
    int a = 2;
labelB:
    //...
    resume(); // goback
labelC:
    //...
    resume(); // goback
}
```

现在编译器显然是不支持这种能力的。那如果我想让编译器支持这种能力，我需要怎么做呢？当在`funA`调用 goto 的时候，`a`的值是应该是多少呢？显然，应该也只能是`2`。而`a`的值是存储在哪里的？在函数栈帧当中。

首先简单回顾一下函数调用和函数栈帧。首先函数是根据代码段去顺序执行的，然后会调用另一个函数（实际信息就是一个代码段的地址），即告诉 cpu 下一段代码在哪里，跳转过去，但是函数执行完需要回来，因此需要记录当前代码段地址。除此之外，还可能需要向另一个函数传入参数，因此还需要记录参数的值。

那也就是说，当在`funA`调用 `goto labelB`的时候，也许有一个类似函数调用和函数栈帧的机制去记录当前调用位置以及上下文。

同样地，一个朴素的想法就还是接着使用栈帧记录。看见`goto labelB`的时候就继续压入当前地址，还需要压入什么呢？

目前 c++关于协程的能力还是比较原始的，这是因为 c++讲究的是零抽象开销，高性能。因此 C++20 的协程接口都是非常底层的。外部需要使用

C++20 引入了对协程的原生支持，使得编写异步代码和生成器变得更加简洁和高效。协程允许函数在执行过程中暂停，并在稍后恢复执行，从而实现非阻塞的异步操作。下面是一个简单的 C++20 协程示例，展示了如何使用协程实现一个生成器函数，该函数生成一系列整数。

```cpp
#include <coroutine>
#include <iostream>
#include <optional>
#include <thread>
#include <chrono>
// 协程生成器
template<typename T>
class Generator {
public:
    struct promise_type {
        T current_value;
        std::suspend_always yield_value(T value) {
            current_value = value;
            return {};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        Generator get_return_object() {
            return Generator{ std::coroutine_handle<promise_type>::from_promise(*this)      };
        }
        void return_void() {}
        void unhandled_exception() { std::exit(1); }
    };
    using handle_type = std::coroutine_handle<promise_type>;
    explicit Generator(handle_type h) : coro(h) {}
    ~Generator() { if (coro) coro.destroy(); }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept : coro(other.coro) {
        other.coro = nullptr;
    }
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (coro) coro.destroy();
            coro = other.coro;
            other.coro = nullptr;
        }
        return *this;
    }
    class iterator {
    public:
        void operator++() { coro.resume(); }
        const T& operator*() const { return coro.promise().current_value; }
        bool operator==(std::default_sentinel_t) const { return coro.done(); }
        explicit iterator(handle_type h) : coro(h) {}
    private:
        handle_type coro;
    };
    iterator begin() {
        coro.resume();
        return iterator{ coro };
    }
    std::default_sentinel_t end() { return {}; }
private:
    handle_type coro;
};
// 生成器函数
Generator<int> range(int start, int end, int step = 1) {
    for (int i = start; i < end; i += step) {
        co_yield i;
    }
}
int main() {
    std::cout << "范围生成器 (0-10, 步长1): ";
    for (auto value : range(0, 10, 1)) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "范围生成器 (1-10, 步长2): ";
    for (auto value : range(1, 10, 2)) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

### 主要关键字

- `co_yield`：用于在协程中生成一个值，并暂停协程的执行，等待下一次恢复。
- `co_await`：用于等待一个异步操作完成，暂停协程的执行，直到操作完成后恢复。
- `co_return`：用于从协程中返回一个值，结束协程的执行。

协程的创建相关关键字：

- `std::coroutine_handle`：表示协程的句柄，可以用于控制协程的执行。
- `promise_type`：协程的承诺类型，定义了协程的行为和状态管理。

协程的管理相关关键字：

- `initial_suspend`：协程开始时的挂起点。
- `final_suspend`：协程结束时的挂起点。
- `yield_value`：定义协程生成值时的行为。

`suspend_always` 和 `suspend_never`：

- `std::suspend_always`：表示协程在该点总是挂起。
- `std::suspend_never`：表示协程在该点从不挂起。

## c++20 协程

[C++20-Coroutines-cppreference](https://en.cppreference.com/w/cpp/language/coroutines)开篇就一句话总结了：协程是“可以暂停和恢复的函数”。这句话很好地概括了协程的核心概念。

以下是三个使用协程的场景：

- 网络 I/O：回调地狱写法（多层嵌套、错误处理分散）

```cpp
// callback 风格（伪）
void async_read_some(Socket sock, void* buf, size_t len,
                     function<void(size_t)> on_success,
                     function<void(Error)> on_error)
{
    // 1. 注册到事件循环（如果还没注册）
    event_loop.register_read_interest(sock, [sock, buf, len, on_success, on_error]{
        ssize_t n = ::recv(sock.fd, buf, len, 0); // 非阻塞 recv
        if (n > 0) {
            on_success((size_t)n);
        } else if (n == 0) {
            on_success(0); // EOF
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 可能是竞态：继续等待，下次会回调
                return;
            }
            on_error(make_error(errno));
        }
    });
}

// 伪代码，仅演示结构
void handle_connection(Socket socket) {
    char buf[1024];
    socket.async_read_some(buf, 1024, [&](size_t n1){
        parse_request(buf, n1, [&](Request req){
            db.async_get_user(req.user_id, [&](User u){
                auto bytes = render_response(u);
                socket.async_write(bytes.data(), bytes.size(), [&](size_t){
                    // done
                }, [&](Error e){ log(e); close(socket); });
            }, [&](Error e){ log(e); close(socket); });
        }, [&](Error e){ log(e); close(socket); });
    }, [&](Error e){ log(e); close(socket); });
}
```

- 网络 I/O：协程写法（顺序表达、集中错误处理）

```cpp
task<> handle_connection(Socket socket) {
    char buf[1024];
    try {
        for (;;) {
            size_t n = co_await socket.async_read_some(buf, 1024);
            Request req = parse_request(buf, n);
            User u = co_await db.async_get_user(req.user_id);
            auto bytes = render_response(u);
            co_await socket.async_write(bytes.data(), bytes.size());
        }
    } catch (Error e) {
        log(e);
        close(socket);
    }
    co_return;
}
```

- 磁盘 I/O：行读取生成器（逐行产出，边读边处理）

```cpp
generator<std::string> read_lines(File f) {
    char buf[4096];
    std::string pending;
    for (;;) {
        size_t n = co_await f.async_read(buf, sizeof(buf));
        if (n == 0) break;
        auto chunk = pending + std::string(buf, buf + n);
        size_t start = 0;
        for (size_t i = 0; i < chunk.size(); ++i) {
            if (chunk[i] == '\n') {
                co_yield chunk.substr(start, i - start);
                start = i + 1;
            }
        }
        pending = chunk.substr(start); // 保留残余半行
    }
    if (!pending.empty()) co_yield pending;
    co_return;
}
```

- 磁盘 I/O：消费生成器（写法接近同步思维）

```cpp
task<> process_file(Path p) {
    File f = co_await open_async(p);
    for (auto line : read_lines(f)) {
        co_await process_line_async(line);
    }
    co_return;
}
```

```cpp
// A:
task<> tcp_echo_server()
{
    char data[1024];
    while (true)
    {
        std::size_t n = co_await socket.async_read_some(buffer(data));
        co_await async_write(socket, buffer(data, n));
    }
}

// B:
generator<unsigned int> iota(unsigned int n = 0)
{
    while (true)
        co_yield n++;
}

// C:
lazy<int> f()
{
    co_return 7;
}
```

### 1. 协程的原理

因为协程的目的是为了实现“可以暂停和恢复的”函数，为了实现这个功能，一个朴素的思想方式其实就是跨函数的`goto`。当前函数执行到一半，就可以使用`goto`跳转到另一个函数，然后再跳转回来。

```c++
void funA(){
    //...
    int a = 1;
    goto labelB
    //...
    goto labelC
}

void funB(){
    int a = 2;
labelB:
    //...
    resume(); // goback
labelC:
    //...
    resume(); // goback
}
```

现在的编译器并不支持“跨函数 goto”。若真要实现，首先要回答两个问题：1) 从 funA 跳到 funB 后再回来，funB 中的局部变量（如 a=2）如何在恢复时保持正确值？2) 这些需要保持的值与返回位置存放在哪里？传统函数调用依靠栈帧保存返回地址、参数和局部变量并在一次线性调用结束后整体回收；而“可暂停/恢复”要求在函数尚未“结束”时长期保留执行位置与活跃局部状态。

直接沿用普通栈帧去做“跨函数暂停”意味着：在每个伪 goto（挂起点）处手动保存当前指令地址、当前活跃局部变量、必要的寄存器，并在恢复时重新装载。这会与平台 ABI、寄存器分配、异常与对象析构顺序深度耦合，复杂且难以移植。

首先是因为，传统 ABI 假定调用者把返回地址/参数/保存的寄存器放在栈或寄存器的既定位置。任意修改栈帧布局或把“返回点”搬到别处会让编译器产生的假设（比如尾调用、内联恢复、栈回溯）失效，导致返回到错误地址或破坏返回值传递。

其次，局部对象的生命周期与异常处理语义会变得不透明，难以保证析构时机正确。再者，不同平台的栈布局、红区保护、寄存器调用约定差异巨大，直接操纵栈帧不可移植且易出错。

于是编译器采用更安全的策略：把原本隐式的控制流转换成显式状态机，并将需要跨挂起点存活的变量提升到一个独立的协程帧对象中。

演化路径概述：

- 直觉（跨函数 goto）抽象成三个要素：

  1. “继续执行位置”标识（下一段代码的标签/地址）
  2. “需跨挂起点保留的数据”存储区（原本在栈帧里）
  3. “恢复时重新跳回并重建上下文”的操作

- 放弃真实栈切换的原因：

  - 修改/切换调用栈与返回地址易破坏既有 ABI 与优化。
  - 局部对象的生命周期与异常栈展开语义会变得不透明。
  - 不同平台的栈布局、红区、寄存器调用约定差异大。

- 编译器替代方案（状态机 + 协程帧）核心动作：
  1. 为每个挂起点分配整数状态编号。
  2. Hoist：挑出需要跨挂起点继续存在的局部（及必要临时）并存入 frame 结构。
  3. 重写原函数：首次调用仅创建并初始化 frame；后续 resume 进入一个 switch(state) 分派的入口。
  4. 在挂起点生成保存 state、保存必要值、构造/使用 awaiter 以决定是否真正挂起或直接继续。
  5. 提供 resume/destroy：resume 根据 state 继续执行；destroy 负责析构 frame 中对象与释放内存。

伪代码示意（简化）：

```cpp
// 原始
task<int> foo() {
    int x = 1;
    co_await A();      // S1
    x += 2;
    co_await B();      // S2
    co_return x;
}

// 变换后骨架
struct frame {
    int state;   // 0 初始, 1 恢复至 S1 后, 2 恢复至 S2 后, 3 结束
    int x;
    // promise 等元数据...
};

task<int> foo() {
    frame* f = allocate();
    f->state = 0;
    f->x = 1;
    return task{f}; // initial_suspend 决定是否马上执行
}

void resume(frame* f) {
    switch (f->state) {
    case 0:
        f->state = 1;
        if (A_await_suspend(f)) return; // 挂起则退出
        // 已准备继续
    case 1:
        f->x += 2;
        f->state = 2;
        if (B_await_suspend(f)) return;
    case 2:
        set_result(f, f->x);
        f->state = 3; // final_suspend 可再次挂起供外部销毁
        return;
    }
}
```

要点强化：

- “无栈”含义：不分配独立调用栈；仅保留最小必要状态于帧，减少切换成本并规避栈布局差异。
- 局部对象生命周期：被提升进 frame 后，其析构延后到 destroy（或 final_suspend 后的销毁），不再与语句块结束或函数返回严格同步。
- 直觉到实现的映射：
  - goto 标签 → 状态编号
  - 原栈帧里跨挂起点需保留的局部 → frame 成员
  - 手动保存/恢复寄存器与返回地址 → 编译器生成的状态转移 + awaiter 协议
  - 回跳逻辑 → resume(switch(state))

精炼结论：跨函数 goto 的思路被重构为“有限状态编号 + 协程帧对象 + 自动生成的 switch 分派”，通过 awaiter/promise 协议封装挂起与恢复细节，避免直接操纵真实调用栈的不可移植性与语义风险。

1. 编译器在编译期把函数拆成若干片段，并给每个挂起点分配状态号；把需要跨挂起点保持的局部提升为 frame 成员。
2. 到达 co_yield：会先把要产出的值放到 promise/frame（或通过 awaiter 传递），然后设置 frame->state 为下一个状态号；如果需要挂起，调用 await_suspend/或在 yield_path 中返回控制权。
3. 挂起发生后，原来的“函数栈帧”上活着的那些只在挂起前短期使用的局部会随着普通函数返回、其对应的栈帧会被弹出；但那些被提升的变量仍然保存在协程帧上，供以后 resume 使用。
4. 事件/调用者在未来某时调用 coroutine_handle.resume()，resume 进入生成的 switch(state) 分支，继续执行，使用存于 frame 的变量恢复语义。
5. 最终销毁时（destroy 或 final_suspend 后），frame 中的对象被析构并释放内存。
