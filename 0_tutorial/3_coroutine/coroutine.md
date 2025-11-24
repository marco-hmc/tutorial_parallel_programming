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
