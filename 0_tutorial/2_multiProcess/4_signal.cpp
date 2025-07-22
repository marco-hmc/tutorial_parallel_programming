#include <csignal>
#include <iostream>

/*
  - 通过系统调用接口给特定进程发送信号

    ```c++
    #include<signal.h>

    int kill(pid_t pid, int signo);
    //向特定进程发送特定信号;成功返回0;失败返回-1

    int raise(int signo);
    //向当前进程发送特定信号;成功返回0;失败返回-1

    #include<stdlib.h>
    void abort(void);
    //使当前进程收到信号而异常终止；就像exit()函数一样，abort()函数总是会成功的，所以没有返回值
    ```

  - 由软件条件发送信号

    - SIGPIPE：SIGPIPE 是一种由软件条件产生的信号，**当一个管道的读端被关闭时，这时候操作系统就会检测到该管道中写入的数据不会在有人来管道内读文件了，操作系统会认为该管道的存在会造成内存资源的极大浪费，则操作系统就会向写端对应的目标进程发送 SIGPIPE 信号**

    - 定时器

      ```c++
      #include<unistd.h>
      unsigned int alarm(unsigned int seconds);
      //调用alarm函数可以对当前进程设置一个闹钟，也就是告诉操作系统在seconds秒之后对当前进程发送SIGALRM信号，该信号的默认处理动作是终止当前进程
      ```

- **信号集操作函数**

  ```c++
  #include<signal.h>

  //注意：在使用sigset_t类型的变量前，一定要调用sigemptyset或sigfillset进行初始化，使信号集处于某种确定的状态，初始化之后就可以调用sigaddset或sigdelset在信号集中添加或删除某种有效信号

  int sigemptyset(sigset_t *set);
  //初始化set所指向的信号集，使其中所有信号对应的比特位清零，表示该信号集不包含任何信号

  int sigfillset(sigset_t *set);
  //初始化set所指向的信号集，将其中所有信号对应的比特位置1，表示该信号集的有效信号包括系统支持的所有信号

  int sigaddset(sigset_t *set, int signo);
  //表示将set所指向的信号集中的signo信号置1

  int sigdelset(sigset_t *set, int signo);
  //表示将set所指向的信号集中的signo信号清零

  int sigismember(const sigset_t *set, int signo);
  //用来判断set所指向的信号集的有效信号中是否包含signo信号，包含返回1，不包含返回0，出错返回-1

  int sigpending(sigset_t *set);
  // 获取进程的pending信号集
  // 成功返回0；失败返回-1
  ```

- **设置/修改进程的信号屏蔽字（block 表）**

  ```c++
  #include<signal.h>

  int sigprocmask(int how, const sigset_t *set, sigset_t *oset);

  
    int how：
        SIG_BLOCK：set包含了用户希望添加到当前信号屏蔽字的信号，即就是在老的信号屏蔽字中添加上新的信号。相当于：mask=mask|set
        SIG_UNBLOCK：set包含了用户希望从当前信号屏蔽字中解除阻塞的信号，即就是在老的信号屏蔽字中取消set表中的信号。相当于：mask=mask&~set
        SIG_SETMASK：设置当前进程的信号屏蔽字为set所指向的信号集。相当于：mask=set
    const sigset_t *set：
        将要设置为进程block表的信号集
    sigset_t *oset：
        用来保存进程旧的block表
        若无需保存进程旧的block表，传递空指针即可
  

  #include<signal.h>

  struct sigaction
  {
      void (*sa_handler)(int);	//指向信号处理对应的函数
      void (*sa_sigaction)(int, siginfo_t *, void *);
      sigset_t sa_mask; //当在处理所收到信号时，想要附带屏蔽的其他普通信号，当不需要屏蔽其他信号时，需要使用sigemptyset初始化sa_mask
      int sa_flags;
      void (*sa_restorer)(void);
  };

  int sigaction(int signo, const struct sigaction *act, struct sigaction *oact);
  
  int signo：
    指定的信号编号
  const struct sigaction *act：
    若该act指针非空，则根据act指针来修改进程收到signo信号的处理动作
  struct sigaction *oact：
    若oact指针非空，则使用oact来保存信号旧的处理动作
*/

void signalHandler(int signal) {
    std::cout << "Received signal: " << signal << '\n';
}

int main() {
    signal(SIGINT, signalHandler);

    std::cout << "Running..." << '\n';

    while (true) {
    }

    return 0;
}