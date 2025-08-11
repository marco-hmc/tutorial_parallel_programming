#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

/*
4. **共享内存的使用步骤**

   - **分配共享内存**：进程首先需要使用 `shmget` 函数分配共享内存。该函数用于创建或获取一个共享内存段，成功时返回共享内存 ID，失败返回 -1。其原型为：

   ```c
   #include <sys/shm.h>
   int shmget(key_t key, size_t size, int flag);
   ```

   其中，`key` 是一个用于标识共享内存的键值，可通过 `ftok` 函数生成，它在系统范围内唯一标识共享内存；`size` 为共享内存的大小（字节数），当创建新的共享内存时必须指定其大小，若引用已存在的共享内存，则可将 `size` 指定为 0；`flag` 用于指定共享内存的访问权限和创建标志等，例如 `IPC_CREAT` 表示若共享内存不存在则创建它，`IPC_EXCL` 与 `IPC_CREAT` 一起使用可确保创建的共享内存是新的，若已存在则返回错误。 - **连接共享内存**：随后，需要访问该共享内存块的每个进程都要使用 `shmat` 函数将共享内存连接到自己的地址空间。连接成功后，可像访问本地内存空间一样对共享内存进行操作。函数原型为：

   ```c
   void *shmat(int shm_id, const void *addr, int flag);
   ```

   其中，`shm_id` 是由 `shmget` 函数返回的共享内存 ID；`addr` 通常设为 `NULL`，表示由系统自动选择合适的地址进行映射；`flag` 用于指定映射的方式，如 `SHM_RDONLY` 表示以只读方式映射。成功时返回指向共享内存的指针，失败返回 -1。 - **断开连接与释放**：当进程完成通信后，使用 `shmdt` 函数断开与共享内存的连接。注意，这只是使当前进程不再访问该共享内存，并非从系统中删除共享内存。函数原型为：

   ```c
   int shmdt(void *addr);
   ```

   其中，`addr` 是 `shmat` 函数返回的指向共享内存的指针，成功返回 0，失败返回 -1。最后，由一个进程使用 `shmctl` 函数并指定 `cmd` 参数为 `IPC_RMID` 来从系统中删除该共享内存块。`shmctl` 函数还可对共享内存执行其他操作，其原型为：

   ```c
   int shmctl(int shm_id, int cmd, struct shmid_ds *buf);
   ```

   `shm_id` 为共享内存 ID，`cmd` 为要执行的操作命令，`buf` 是一个指向 `struct shmid_ds` 结构体的指针，用于获取或设置共享内存的相关信息，具体取决于 `cmd` 的值。

5. **mmap 实现共享内存**

   - **mmap 基本功能**：`mmap` 系统调用并非专门为共享内存设计，它提供了一种不同于普通文件访问的方式，使进程能够像读写内存一样操作普通文件。不过，利用 `mmap` 实现共享内存是其主要应用之一。
   - **共享内存实现方式**：`mmap` 系统调用通过将同一个普通文件映射到多个进程的地址空间，实现进程之间的共享内存。普通文件被映射后，进程可直接对映射的内存区域进行读写，无需再调用 `read()` 和 `write()` 等函数。例如：

   ```c
   #include <sys/mman.h>
   void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
   ```

   `addr` 通常设为 `NULL`，由系统自动选择映射地址；`length` 是映射的长度；`prot` 指定映射内存的保护方式，如 `PROT_READ`（可读）、`PROT_WRITE`（可写）等；`flags` 用于指定映射的特性，如 `MAP_SHARED` 表示共享映射，使对映射区域的修改会反映到文件中；`fd` 是要映射文件的文件描述符；`offset` 是映射的偏移量，通常设为 0。 - **数据同步与限制**：进程对映射内存的修改不会立即写入文件，需要调用 `msync` 函数进行同步，将内存中的修改写回文件。`msync` 函数原型为：

   ```c
   int msync(void *addr, size_t length, int flags);
   ```

   其中，`addr` 是映射内存的起始地址，`length` 是要同步的长度，`flags` 可选择 `MS_SYNC`（同步写回，等待写操作完成）、`MS_ASYNC`（异步写回）等。此外，使用 `mmap` 映射文件时，映射的长度在调用 `mmap` 时就已确定，一般情况下无法直接通过这种方式增加文件的长度，因为它是基于现有文件大小进行映射的。如果需要增加文件长度，通常需要先扩展文件，再重新映射。

   综上所述，共享内存为进程间通信提供了高效的方式，但在使用时需注意同步与资源管理等问题，以确保数据的一致性和系统的稳定性。

*/

/*

1. **`shmget()`的使用**：

   `shmget()`函数用于创建或访问一个共享内存段。它的原型如下：
   ```cpp
   int shmget(key_t key, size_t size, int shmflg);
   ```

    `key`：共享内存的键值。可以指定一个具体的值，或者使用`IPC_PRIVATE`创建一个新的共享内存段。
    - `size`：共享内存段的大小，以字节为单位。

    `shmflg`：权限标志，通常是权限位（如`0644`）与`IPC_CREAT`、`IPC_EXCL`等标志的组合。
    返回值是共享内存段的标识符（ID），用于后续的操作。

2. **`shmat()`的使用**：

   `shmat()`函数将共享内存段连接到进程的地址空间中。它的原型如下：
   ```cpp
   void *shmat(int shmid, const void *shmaddr, int shmflg);
   ```
   - `shmid`：`shmget()`返回的共享内存标识符。
   -
    `shmaddr`：指定共享内存连接到进程地址空间中的具体地址。通常设置为`NULL`，让系统自动选择地址。
    - `shmflg`：操作标志，通常是`0`或`SHM_RDONLY`（只读连接）。
    返回值是指向共享内存段第一个字节的指针。

3. **`shmdt()`的使用**：
   `shmdt()`函数用于断开共享内存段与当前进程地址空间的连接。它的原型如下：
   ```cpp
   int shmdt(const void *shmaddr);
   ```
   - `shmaddr`：`shmat()`返回的地址指针。
   返回值为`0`表示成功，`-1`表示失败。

4.
**`shmctl()`的使用**：
   `shmctl()`函数用于对共享内存段执行各种控制操作。它的原型如下：
   ```cpp
   int shmctl(int shmid, int cmd, struct shmid_ds *buf);
   ```
   - `shmid`：共享内存标识符。

    `cmd`：控制命令，如`IPC_STAT`（获取共享内存的状态）、`IPC_SET`（设置共享内存的参数）、`IPC_RMID`（删除共享内存段）等。
    - `buf`：指向`shmid_ds`结构的指针，用于存储或设置共享内存的状态信息。
    返回值为`0`表示成功，`-1`表示失败。

5. **`IPC_PRIVATE`和`IPC_CREAT`的含义**：

    `IPC_PRIVATE`：创建一个新的共享内存段。即使键值相同，每次调用`shmget()`时也会创建一个新的共享内存段。
    -
    `IPC_CREAT`：与键值相对应的共享内存段不存在时，创建一个新的共享内存段。如果该共享内存段已存在，则返回现有的ID。通常与权限位一起使用，如`IPC_CREAT
    | 0666`。
*/

int main() {
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        std::cerr << "Failed to create shared memory" << '\n';
        return 1;
    }

    int *sharedData = (int *)shmat(shmid, nullptr, 0);
    if (sharedData == (int *)-1) {
        std::cerr << "Failed to attach shared memory" << '\n';
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to create child process" << '\n';
        return 1;
    }

    if (pid == 0) {
        *sharedData = 42;
        std::cout << "Child process wrote data to shared memory" << '\n';
    } else {
        sleep(1);
        std::cout << "Parent process read data from shared memory: "
                  << *sharedData << '\n';

        if (shmdt(sharedData) == -1) {
            std::cerr << "Failed to detach shared memory" << '\n';
            return 1;
        }

        if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
            std::cerr << "Failed to delete shared memory" << '\n';
            return 1;
        }
    }

    return 0;
}
