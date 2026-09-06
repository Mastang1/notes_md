# Chapter 26 - Zephyr Scheduling as a Running System: Thread, Priority and Wakeup

> Week 4 / Day 5 - 用实验把 ready/block/wakeup/context switch 从源码概念变成运行现象。

[← Part README](README.md) · [← Previous](ch25_kernel_oops_debug.md) · [Next →](ch27_zephyr_memory_thread_analyzer.md)

## 26.1 从 FreeRTOS 迁移到 Zephyr：不要重新学“任务是什么”，要验证 scheduler 的状态机

你已经分析过 FreeRTOS intrusive list/ready list。Zephyr 本章只回答：thread 在哪些状态之间迁移，priority/timeout/synchronization 如何改变 ready set，scheduler 何时发生 context switch。

```text
READY --scheduled--> RUNNING
RUNNING --sem take unavailable--> PENDING/BLOCKED
RUNNING --sleep--> SLEEPING
ISR/thread --sem give--> blocked thread READY
priority decision -> next RUNNING
```

具体内部数据结构不要求今天通读源码。

## 26.2 Priority 先看语义，不背数字

Zephyr 有 cooperative/preemptive priority ranges，具体数值范围由 config 决定。首先使用宏/官方定义理解“更高优先级”的比较语义，避免从 FreeRTOS 的数字方向直接类比。

实验时打印/记录实际 priority：

```c
k_thread_priority_get(k_current_get());
```

## 26.3 Worked Example：三个 thread 构造可解释的调度

角色：

```text
producer  periodic produce message
worker    waits semaphore/message
logger    lower priority periodic report
```

不要三个 thread 都 while(1) printk，那只能制造串口竞争。

Producer：每 100ms push/counter + give sem；Worker 阻塞 take，醒来处理；Logger 每 1s 汇总次数。

通过 timestamp 日志验证：Worker 没事件时不占 CPU；event 到来后按 priority 及时运行。

## 26.4 Semaphore：它表达“可用资源/事件计数”，不是消息内容

对比：

```text
semaphore -> count/event token
msgq      -> copies fixed-size message payload
fifo      -> queues data items/pointers per Zephyr semantics
```

如果你用 semaphore 再靠全局变量传 payload，就必须额外处理数据一致性；不要让“同步”和“数据传输”概念混在一起。

## 26.5 Guided Lab A：优先级反转的最小认知

创建 low thread 持 mutex、high thread 等 mutex、medium thread 消耗 CPU，观察行为。然后查 Zephyr mutex priority inheritance 语义。

不要求今天做实时性论文；目标是知道“mutex 与 semaphore 的 scheduler interaction 不只是 API 名字不同”。

## 26.6 Guided Lab B：timeout 改变 thread state

```c
k_sem_take(&sem, K_MSEC(200));
```

分别让 producer 在 100ms/500ms give，观察 success/timeout。记录 state transition。

## 26.7 Timer 回调 context：不能把所有工作塞进 timer expiry

根据 Zephyr 当前 timer 文档确认 expiry function 执行 context/限制。设计原则仍是：时间事件触发 -> signal/work -> thread context 做较重工作。

将你 MCU ISR “短 ISR + task deferred work”的经验迁移过来。

## 26.8 Independent Challenge：不用源码复述 scheduler decision

给出场景：

```text
T1 high blocked on sem
T2 medium ready
T3 low running
ISR gives sem
```

解释之后哪个 thread 应该获得运行机会、哪些条件影响抢占。再用实验日志验证，不靠想象。

## 26.9 下一章：scheduler 能正确运行不代表产品稳定，thread stack 才是 RTOS 最常见预算事故之一

Chapter 27 把 stack/heap 从“内存 API”升级为可量化 resource budget，用 thread analyzer 和故障注入观察每个 thread 的真实消耗。

## References and manuals

### Zephyr Threads
- Online: [Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)
- 本章阅读定位：重点 thread state、priority、scheduling。

### Zephyr Synchronization
- Online: [Zephyr Synchronization](https://docs.zephyrproject.org/latest/kernel/services/synchronization/index.html)
- 本章阅读定位：按本章实验查看 semaphore/mutex 行为。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch25_kernel_oops_debug.md) · [Next →](ch27_zephyr_memory_thread_analyzer.md)
