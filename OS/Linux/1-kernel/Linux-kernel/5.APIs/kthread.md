这份笔记专为 Linux 内核开发者设计，旨在帮助你快速掌握内核线程（kthread）的开发流及避坑指南。

# 🛠️ Linux Kernel Thread (kthread) 开发速查笔记

## 一、 核心 API 详解

内核线程的实现定义在 `<linux/kthread.h>` 头文件中。以下是最常用的核心 API：

### 1. 线程创建与启动

- **`kthread_create(threadfn, data, namefmt, ...)`**
    
    - **功能**：创建一个内核线程，但线程创建后处于不可运行（TASK_INTERRUPTIBLE）的停止状态。
        
    - **参数**：
        
        - `threadfn`：线程入口函数，形式为 `int threadfn(void *data)`。
            
        - `data`：传递给线程函数的参数。
            
        - `namefmt`：线程名称的格式化字符串（会在 `ps -ef` 中显示，常带中括号如 `[my_kthread]`）。
            
    - **返回值**：成功返回指向 `task_struct` 的指针，失败返回错误指针（需用 `IS_ERR()` 宏判断）。
        
    - **注意**：需要显式调用 `wake_up_process(tsk)` 来唤醒并运行它。
        
- **`kthread_run(threadfn, data, namefmt, ...)`**
    
    - **功能**：这是一个宏，相当于 `kthread_create()` + `wake_up_process()`。
        
    - **快捷性**：创建后**立即投入运行**，最常用。
        
- **`kthread_bind(struct task_struct *k, unsigned int cpu)`**
    
    - **功能**：将创建好但**尚未运行**的线程绑定到指定的 CPU 核心上（亲和性设置）。
        

### 2. 线程状态检查与停止

- **`kthread_should_stop(void)`**
    
    - **功能**：在线程函数内部调用，检查外部是否有人调用了 `kthread_stop()` 请求该线程退出。
        
    - **返回值**：如果收到停止信号返回 `true`，否则返回 `false`。
        
- **`kthread_stop(struct task_struct *k)`**
    
    - **功能**：由外部（如 `module_exit`）调用，通知并**阻塞等待**指定的内核线程退出。
        
    - **原理**：它会设置线程的 `to_idx` 标志，向其发送唤醒信号，并**阻塞等待线程函数返回**。
        
    - **返回值**：返回内核线程入口函数的返回值。
        

## 二、 快速开发模板（Demo）

这是一个标准的内核模块示例，展示了如何正确创建、运行和销毁一个内核线程。

C

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h> // kthread 核心头文件
#include <linux/delay.h>   // msleep

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Expert");
MODULE_DESCRIPTION("A standard kthread development template");

static struct task_struct *my_task = NULL;

// 1. 线程入口函数
static int my_thread_worker(void *data)
{
    int count = 0;
    char *thread_name = (char *)data;

    pr_info("kthread: '%s' started\n", thread_name);

    // 2. 核心循环：必须检查 kthread_should_stop()
    while (!kthread_should_stop()) {
        
        // 执行实际的内核业务逻辑
        pr_info("kthread: Working... count = %d\n", count++);

        // 3. 必须主动让出 CPU（睡眠或调度），防止死锁/占用满 CPU
        msleep(1000); 
    }

    pr_info("kthread: '%s' exiting\n", thread_name);
    return 0; // 返回值会被 kthread_stop 接收
}

// 模块加载
static int __init my_kthread_init(void)
{
    pr_info("kthread_module: Initializing\n");

    // 使用 kthread_run 创建并立即运行线程
    my_task = kthread_run(my_thread_worker, "my_demo_info", "demo_kthread");
    
    if (IS_ERR(my_task)) {
        pr_err("kthread_module: Failed to create thread\n");
        return PTR_ERR(my_task);
    }

    return 0;
}

// 模块卸载
static void __exit my_kthread_exit(void)
{
    pr_info("kthread_module: Exiting\n");

    if (my_task) {
        // 4. 同步等待线程退出，防止野指针和内存泄漏
        int ret = kthread_stop(my_task);
        pr_info("kthread_module: Thread stopped with code %d\n", ret);
        my_task = NULL;
    }
}

module_init(my_kthread_init);
module_exit(my_kthread_exit);
```

## 三、 内核线程开发中的致命“坑”（Pitfalls）

内核态没有用户态 `pthread` 那样的全自动保护机制，代码逻辑稍有不慎就会导致系统卡死或崩溃。以下是生产环境中最容易踩的四大深坑：

### 坑 1：不加控制的“死循环”导致 CPU 100% 占满（Soft Lockup）

- **现象**：系统响应变慢，内核报 `BUG: soft lockup - CPU#x stuck for 22s!`。
    
- **原因**：内核线程默认是**不可抢占的（在未开启内核抢占 CONFIG_PREEMPT 的系统上）**，或者即使开启了抢占，若没有在 `while(!kthread_should_stop())` 循环体内调用能引起阻塞/睡眠的函数（如 `msleep()`、`schedule()`、`wait_event()`），它会疯狂独占该 CPU。
    
- **解法**：循环体内必须有主动让出 CPU 的逻辑。如果是周期性任务，用 `msleep()`；如果是事件驱动，用等待队列 `wait_event_interruptible()`。
    

### 坑 2：`kthread_stop()` 导致系统永久死锁（Hang住）

- **现象**：执行 `rmmod` 卸载模块时，终端直接卡死，Ctrl+C 无效。
    
- **原因**：`kthread_stop()` 是一个**同步阻塞**函数，它会一直等待线程函数 `return`。如果你的线程函数内部发生了以下情况，就会导致死锁：
    
    1. 没有检查 `kthread_should_stop()`，导致循环永远无法打破。
        
    2. 线程内部正阻塞在某个无法被唤醒的等待队列中（例如使用了 `wait_event()` 而不是 `wait_event_interruptible()`，或者唤醒条件永远不满足）。
        
- **解法**：
    
    - 保证循环条件里严格包含 `!kthread_should_stop()`。
        
    - 如果使用了等待队列，必须在唤醒条件中加上 `kthread_should_stop()`，例如：
        
        `wait_event_interruptible(wq, condition || kthread_should_stop());`
        

### 坑 3：重复调用 `kthread_stop()` 或对已退出的线程调用（野指针崩溃）

- **现象**：内核触发 `Oops: Unable to handle kernel paging request`，系统崩溃。
    
- **原因**：如果内核线程自己执行完毕退出了（比如遇到错误主动 `return` ），那么它对应的 `task_struct` 结构体可能会被内核释放。此时如果外部再次调用 `kthread_stop(my_task)`，传入的就是一个**野指针**。
    
- **解法**：
    
    - 如果线程需要自主退出，通常需要配合 `completion` 机制通知父进程，或者在退出前不要让系统自动销毁 `task_struct`（使用 `get_task_struct()`）。
        
    - 最安全的做法是：让内核线程**永远不要自己 return 退出**，其生命周期完全由 `kthread_should_stop()` 和外部的 `kthread_stop()` 掌控。
        

### 坑 4：误在内核线程中使用用户态信号（Signal）

- **现象**：尝试用 `pthread` 的思路向内核线程发送信号（如 `SIGKILL`），结果线程毫无反应。
    
- **原因**：内核线程默认**忽略所有信号**。如果你想在内核线程中响应信号，必须显式调用 `allow_signal()` 允许接收某些信号，并使用 `signal_pending(current)` 进行检查。
    
- **解法**：尽量避免在内核线程中引入用户态信号机制。若需要线程间通信，使用内核原生的 `complete`、`semaphore` 或 `wait_queue`。
    

为了让你更直观地理解内核线程的完整生命周期管理，以及上述“坑”发生的运行机理，可以通过下方的交互式内核线程仿真器，直观观察不同代码设计下的线程行为和状态机切换。