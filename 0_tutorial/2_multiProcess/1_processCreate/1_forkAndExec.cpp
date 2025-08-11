#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

/*
  1. fork()的返回值是什么
    * 负值：表示创建进程失败。
    * 零：表示当前代码在子进程中执行。子进程中fork()的返回值为0。
    * 正值：表示当前代码在父进程中执行。返回值是新创建的子进程的PID。

  2. 怎么理解fork()做了什么？上下文是怎么保存下来的？
    * fork()会复制当前进程的上下文，包括代码段、数据段、堆栈等。这样子进程就可以继续执行父进程的代码。
    * 子进程会复制父进程的内存映像，但是子进程会有自己的内存空间，父子进程的内存空间是独立的。

  3. 为什么fork()返回两次？
    * fork()返回两次，是因为子进程和父进程都会执行fork()后面的代码。子进程返回0，父进程返回子进程的PID。

  4. 什么时候fork()会失败？
    * fork()失败的原因有很多，比如进程数达到上限、内存不足等。fork()失败时，返回值是-1。
*/

namespace Fork {
    void task() {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Fork failed" << '\n';
        }

        if (pid == 0) {
            std::cout << "This is the child process. PID: " << getpid() << '\n';
        } else {
            std::cout << "This is the parent process. Child PID: " << pid
                      << '\n';
        }
    }

}  // namespace Fork

namespace Vfork {
    void task() {}
}  // namespace Vfork

namespace Clone {
    void task() {}

}  // namespace Clone

/*
  1. **为什么`args`后面要有一个`NULL`，可以是`nullptr`吗？**

      `args`数组的最后一个元素必须是`NULL`，以标识参数列表的结束。
      这是因为`execvp`函数需要知道何时停止处理`args`数组。
      使用`nullptr`代替`NULL`是可以的，也是推荐的做法。

  2. **`execvp`怎么用？各个参数的意义是什么？**

      ```cpp
      int execvp(const char *file, char *const argv[]);
      ```
      -
      `file`：要执行的文件名。
          如果`file`中不包含路径（即不含`/`），则`execvp`会在环境变量`PATH`指定的目录中查找该文件。
      -
      `argv`：一个字符串数组，其中包含要传递给`file`指定的程序的参数。
          `argv[0]`通常是程序的名称，数组的最后一个元素必须是`NULL`，以标识参数列表的结束。

  3. **为什么`execvp()`的函数接受的参数是`(args[0],
  args)`。第二个参数不是包括了第一个参数吗？**

      `execvp`函数的设计遵循UNIX传统，其中`argv[0]`通常是程序的名称，这是一种约定。
      虽然`args`数组的第一个元素（`args[0]`）确实被包含在第二个参数`args`中，但`execvp`函数需要这种格式来正确解析和执行程序。
      这样设计允许程序知道自己是如何被调用的，因为`argv[0]`可以与实际的可执行文件名不同。

  4. **`waitpid`的参数是什么意思？**
      ```cpp
      pid_t waitpid(pid_t pid, int *status, int options);
      ```

      - `pid`：指定要等待的子进程的进程ID。特殊值（如`-1`）表示等待任何子进程。

      `status`：一个指针，指向一个整数，在这里函数会存储子进程的终止状态。通过这个参数，父进程可以了解子进程的退出原因。

      `options`：用于修改`waitpid`行为的选项，例如`WNOHANG`表示非阻塞模式，即如果没有子进程退出，`waitpid`会立即返回0而不是阻塞。

      `waitpid`函数的返回值有几种情况：成功时返回子进程的PID；如果设置了`WNOHANG`且没有子进程退出，则返回0；出错时返回-1。
*/

namespace Exec {
    void task() {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed" << '\n';
            return;
        }

        if (pid == 0) {
            char *args[] = {(char *)"/bin/ls", (char *)"-l", nullptr};
            execvp(args[0], args);
            std::cerr << "Exec failed" << '\n';
            return;
        } else {
            int status;
            waitpid(pid, &status, 0);
            std::cout << "Child finished with status " << status << '\n';
        }
    }
}  // namespace Exec

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed" << '\n';
        return 1;
    }

    if (pid == 0) {
        std::cout << "This is the child process. PID: " << getpid() << '\n';
    } else {
        std::cout << "This is the parent process. Child PID: " << pid << '\n';
    }

    return 0;
}
