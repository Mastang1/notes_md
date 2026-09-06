# Chapter 28 - Integration: Linux Process/Thread vs Zephyr Thread/Context

> Week 4 / Day 7 - 用系统问题而不是 API 名字建立两套 OS 的共同坐标系。

[← Part README](README.md) · [← Previous](ch27_zephyr_memory_thread_analyzer.md)

## 28.1 不再按 API 对比两套 OS，而按“系统问题”对比

错误对比：

```text
Linux pthread_create vs Zephyr k_thread_create
Linux mutex vs Zephyr mutex
```

这种表只能帮记 API。正确对比是问：**系统如何表示执行上下文、地址空间、阻塞、优先级、资源和故障？**

## 28.2 执行实体：Linux process/thread 与 Zephyr thread

| 维度 | Linux | Zephyr on STM32F407 |
|---|---|---|
| Process address space | 独立虚拟地址空间 | 通常单 firmware address space |
| Thread | task/thread sharing mm/resources | kernel thread object |
| User/kernel privilege | 明确 syscall boundary | 取决于 userspace/MPU 配置，常见嵌入式 app 全 kernel mode |
| Dynamic loading | ELF process + modules | firmware build-time image 为主 |
| Scheduling | complex general-purpose + RT classes | embedded priority/thread scheduler |

不要把表背下来；每一项都要能链接回本周实验。

## 28.3 Blocking：两套系统的共同本质是“当前执行实体不再 runnable”

Linux `read/poll/futex` 等可能让 task sleep；Zephyr `k_sem_take`/msgq 等让 thread pending。底层 scheduler 都是在维护 runnable/not runnable 的集合，但对象、policy、规模、内存模型不同。

这个共同点会直接进入后面的 IRQ/waitqueue：

```text
interrupt/event
 -> change state/wakeup
 -> scheduler decision
```

## 28.4 Memory：Linux VA 不是 Zephyr stack budget 的“高级版”

Linux：page table/VM/demand paging/user-kernel separation 是系统设计基础。

当前 F407 Zephyr：物理 RAM 很小，链接布局与 thread stack 静态预算更直接。即使 Zephyr 支持 MPU/userspace，也没有变成 Linux 那样的 MMU/virtual memory system。

所以系统软件工程师必须能切换 mental model，不能把 Linux 的 `malloc/free` 直觉搬到 MCU 产品。

## 28.5 Driver execution context：提前建立后续 Week 的问题框架

未来你会遇到：

```text
Linux process context
Linux hard IRQ/threaded IRQ/workqueue
Zephyr thread context
Zephyr ISR/workqueue
```

关键问题永远是：

- 当前能否 sleep/block？
- 谁拥有资源？
- 能否被抢占？
- 锁该用哪类？
- 栈属于谁？

今天不学答案，先建立提问框架。

## 28.6 Debug 对照：证据层级不同，但方法论相同

### Linux

```text
strace -> /proc/sysfs -> dmesg -> ftrace/perf -> GDB/Oops -> hardware
```

### Zephyr

```text
logging -> shell/analyzer -> GDB/RTOS awareness -> fault dump -> JTAG/register -> hardware
```

共同方法论：**先定位层级，再选择工具；不先猜代码。**

## 28.7 Guided Lab：把前三周产物串起来做一个 15 分钟系统讲解

必须覆盖：

1. 6ULL 从 U-Boot 到 Linux；
2. Linux app exec/ELF/VA/fd；
3. Kernel module build/load；
4. F407 Zephyr board/DT/thread；
5. 两套系统调试入口。

限制自己只能用 5 张图，逼迫模型收敛。

## 28.8 Week 4 Gate

Linux：

- [ ] 能画 Kconfig -> .config -> Kbuild -> vmlinux/zImage/module；
- [ ] `.ko` 可重复 build/load/unload；
- [ ] 能解释 symbol/vermagic/dependency；
- [ ] 完成一次安全 Oops 定位。

Zephyr：

- [ ] 三线程实验可解释 ready/block/wakeup；
- [ ] semaphore/mutex 不再只停留在 API；
- [ ] thread analyzer 给出真实 stack usage；
- [ ] 完成一次 stack fault/near-overflow 实验并恢复。

## 28.9 Part IV 结语：下一阶段进入真正的 DeviceTree/Driver Model

Week 1-4 已经完成“环境 -> BSP -> User Space -> Kernel Module/RTOS runtime”的基础链。接下来原总计划 Week 5 才正式进入 Linux DeviceTree 与 Driver Model：`DTS -> device_node -> platform_device -> match -> probe -> resource`。因为你现在已经知道 Kernel 是怎么构建、module 怎样加入、用户态怎样调用，DeviceTree/Driver Model 会落在一个完整系统里，而不再是孤立结构体。

## References and manuals

### Linux Kernel documentation
- Online: [Linux Kernel documentation](https://docs.kernel.org/)
- 本章阅读定位：回查 Kbuild/module/debug 三条线。

### Zephyr Kernel Services
- Online: [Zephyr Kernel Services](https://docs.zephyrproject.org/latest/kernel/services/index.html)
- 本章阅读定位：回查 thread/synchronization/memory 的官方分类。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch27_zephyr_memory_thread_analyzer.md)
