# FreeRTOS / RTOS 核心理论笔记

> [!info] 文档说明
> 本笔记基于当前 `FreeRTOS/FreeRTOS` 主仓库及其 `FreeRTOS-Kernel` 内核源码整理。
> 参考内核版本：**FreeRTOS Kernel V11.3.1**。
> 重点平台：**ARM Cortex-M4F / GCC Port**。
>
> 本笔记目标不是背 API，而是建立：
>
> **CPU 异常模型 → 上下文切换 → 调度器 → 内核链表 → 同步对象 → 时间系统 → 故障调试**
>
> 的完整心智模型。

---

# 目录

- [[#0. 总体心智模型]]
- [[#1. 上下文切换与 PendSV]]
- [[#2. 临界区与 BASEPRI]]
- [[#3. EventGroup 的 FromISR 为什么是延迟执行]]
- [[#4. Mutex 与优先级继承]]
- [[#5. Heap_4 与 Heap_5]]
- [[#6. HardFault 与栈溢出调试]]
- [[#7. 调度器的本质]]
- [[#8. 为什么阻塞任务需要两个 ListItem]]
- [[#9. FreeRTOS 调度是否 O(1)]]
- [[#10. SysTick 到底做了什么]]
- [[#11. Tick 溢出]]
- [[#12. vTaskDelay、xTaskDelayUntil、xTaskPeriodicDelay]]
- [[#13. 同优先级任务如何调度]]
- [[#14. 临界区、挂起调度器、Mutex 的区别]]
- [[#15. 为什么 ISR 后必须 portYIELD_FROM_ISR]]
- [[#16. configMAX_SYSCALL_INTERRUPT_PRIORITY]]
- [[#17. 零延迟中断应该如何设计]]
- [[#18. 为什么 Task Notification 很快]]
- [[#19. Binary Semaphore 与 Mutex]]
- [[#20. 优先级反转、饥饿、死锁]]
- [[#21. EventGroup 常见故障]]
- [[#22. 软件定时器与 Timer Daemon Task]]
- [[#23. Queue 的数据复制语义]]
- [[#24. StreamBuffer 与 MessageBuffer]]
- [[#25. 动态内存与实时系统]]
- [[#26. 任务栈大小]]
- [[#27. Stack High Water Mark]]
- [[#28. Cortex-M HardFault 标准排查流程]]
- [[#29. vTaskDelete 与内存回收]]
- [[#30. 为什么“内核崩溃”往往是应用层错误]]
- [[#31. SMP 与 AMP]]
- [[#32. FreeRTOS + newlib]]
- [[#33. FreeRTOS + lwIP]]
- [[#34. FreeRTOS + FatFs]]
- [[#35. 实时性的本质]]
- [[#36. 为什么 ISR 必须短]]
- [[#37. 为什么 volatile 不能代替同步机制]]
- [[#38. 为什么 FromISR API 不能阻塞]]
- [[#39. 为什么 FreeRTOS 大量使用链表]]
- [[#40. 为什么调度不依赖 SysTick]]
- [[#41. 面试优先级总结]]
- [[#42. 最终记忆笔记]]
- [[#参考资料]]

---

# 0. 总体心智模型

FreeRTOS 可以先压缩成 5 层：

```text
中断 / 异常
    │
    ▼
内核对象
Queue / Semaphore / Mutex / Notification / EventGroup
    │
    ▼
任务状态管理
Ready / Blocked / Delayed / Suspended
    │
    ▼
调度器
选择最高优先级 Ready Task
    │
    ▼
Port 层
SysTick / PendSV / SVC / BASEPRI / PSP / MSP
```

最值得记住的 5 句话：

1. **Task = TCB + 私有栈 + 链表节点。**
2. **调度器 = 在不同链表之间移动 TCB + 选择最高优先级 Ready Task。**
3. **Blocked 的本质 = 从可运行集合中移除，因此不消耗 CPU。**
4. **Cortex-M 上下文切换 = 保存当前 PSP 上下文到当前 TCB，再从下一个 TCB 恢复上下文。**
5. **FreeRTOS 最难排查的问题，通常来自中断优先级、错误 API 上下文、对象生命周期、内存破坏和阻塞设计错误。**

---

# 1. 上下文切换与 PendSV

## Q1：FreeRTOS 在 Cortex-M 上如何完成任务切换？

### 标准答案

上下文切换的本质不是“调用另一个任务函数”，而是：

> 保存 Task A 的 CPU 执行现场到 Task A 的栈中，修改 `pxCurrentTCB` 指向 Task B，再从 Task B 的栈恢复 CPU 执行现场。

典型流程：

```text
SysTick / ISR / taskYIELD()
        │
        ├─ 判断是否需要调度
        │
        └─ 挂起 PendSV
                 │
                 ▼
        xPortPendSVHandler
                 │
        保存 Task A 上下文
                 │
        pxCurrentTCB = Task B
                 │
        恢复 Task B 上下文
                 │
        异常返回
                 ▼
              Task B
```

### Cortex-M 异常进入时，硬件自动保存什么？

从 Thread Mode 进入异常时，硬件自动压栈：

```text
R0
R1
R2
R3
R12
LR
PC
xPSR
```

对于 Cortex-M4F，如果使用了浮点上下文，还可能存在扩展 FP 栈帧。

FreeRTOS 软件额外保存：

```text
R4-R11
EXC_RETURN
S16-S31（需要时）
```

典型 PendSV 核心逻辑：

```asm
mrs   r0, psp
stmdb r0!, {r4-r11, r14}

ldr   r3, =pxCurrentTCB
ldr   r2, [r3]
str   r0, [r2]

bl    vTaskSwitchContext

ldr   r1, [r3]
ldr   r0, [r1]

ldmia r0!, {r4-r11, r14}
msr   psp, r0
bx    r14
```

所以：

> `pxTopOfStack` 必须放在 TCB 的第一个成员。

因为 PendSV 汇编直接通过 TCB 首地址访问它。

---

## 为什么必须使用 PendSV？

错误说法：

> SysTick 里直接切任务一定会 HardFault。

更准确的说法：

> PendSV 提供了一个统一的、低优先级的、可延迟执行的上下文切换点。

正常设计是：

```text
SysTick：
    更新时间
    唤醒到期任务
    判断是否需要重新调度

PendSV：
    保存当前任务
    选择下一个任务
    恢复下一个任务
```

PendSV 通常设置为最低异常优先级。

这样即使 SysTick 或普通 ISR 发生时系统已经处于别的中断中，也不会在高优先级 ISR 执行中途直接做任务上下文切换。

---

## EXC_RETURN 不能只背 `0xFFFFFFFD`

很多教程只背：

```text
0xFFFFFFFD
```

但对于带 FPU 的 Cortex-M4F，如果使用扩展浮点上下文，还可能出现：

```text
0xFFFFFFED
```

所以 FreeRTOS 会保存并检查 EXC_RETURN。

> [!important] 面试记忆
> **SysTick 主要做时间推进和调度决策，PendSV 负责真正的 CPU 上下文保存与恢复。**
>
> PendSV 的意义是统一、延迟、低优先级执行上下文切换，而不是“SysTick 直接切一定崩溃”。

---

# 2. 临界区与 BASEPRI

## Q2：`taskENTER_CRITICAL()` 到底屏蔽了哪些中断？

在 Cortex-M3/M4/M7 这类支持 `BASEPRI` 的端口上，FreeRTOS 通常不是“全局关所有中断”。

调用链：

```text
taskENTER_CRITICAL()
    ↓
vPortEnterCritical()
    ↓
portDISABLE_INTERRUPTS()
    ↓
vPortRaiseBASEPRI()
    ↓
BASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY
```

假设：

```text
configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5
```

Cortex-M 中：

```text
数值越小，优先级越高
```

可以理解为：

```text
0   最高
1
2   零延迟中断
3
4
------------------------
5   RTOS API 优先级边界
6   会被 BASEPRI 屏蔽
7   会被屏蔽
...
15  最低
```

---

## 两类 ISR

### 1. RTOS-aware ISR

优先级数值满足：

```text
>= configMAX_SYSCALL_INTERRUPT_PRIORITY
```

这些 ISR 可以调用：

```c
xQueueSendFromISR()
xSemaphoreGiveFromISR()
vTaskNotifyGiveFromISR()
...
```

---

### 2. 零延迟 / 高优先级 ISR

优先级数值满足：

```text
< configMAX_SYSCALL_INTERRUPT_PRIORITY
```

这些 ISR 在 FreeRTOS 临界区中仍然可以打断 CPU。

但：

> **绝对不能调用 FreeRTOS 内核 API，包括 FromISR API。**

原因是：

```text
高优先级 ISR
    ↓
打断 FreeRTOS 正在修改 Ready List
    ↓
ISR 又调用 xQueueSendFromISR()
    ↓
再次操作内核链表
    ↓
链表处于中间态
    ↓
内核结构损坏
```

---

## 注意可移植性

这个 BASEPRI 模型并不适用于所有 Cortex-M。

例如 Cortex-M0/M0+ 没有同样的 BASEPRI 能力，端口实现可能采用更大范围的中断屏蔽。

> [!important] 面试记忆
> **BASEPRI 本质是给 FreeRTOS 划了一条“可以调用内核 API 的中断优先级边界”。**
>
> 边界以上的高优先级中断可以保持超低延迟，但必须与内核完全解耦。

---

# 3. EventGroup 的 FromISR 为什么是延迟执行

## Q3：为什么 `xEventGroupSetBitsFromISR()` 和 `xQueueSendFromISR()` 实现完全不同？

`xQueueSendFromISR()` 可以直接在 ISR 中执行队列发送。

大致流程：

```text
进入 ISR 临界区
    ↓
检查队列是否有空间
    ↓
复制数据
    ↓
检查接收等待列表
    ↓
唤醒最高优先级接收任务
    ↓
设置 pxHigherPriorityTaskWoken
```

但说它“严格 O(1)”也不准确。

因为队列复制成本与：

```text
uxItemSize
```

相关。

更严谨的说法：

> 对固定大小 Queue Item，执行时间是有界的；但复制本身仍与 item size 有关。

---

## EventGroup 为什么不直接在 ISR 设置 bit？

因为：

```c
xEventGroupSetBits()
```

可能需要遍历所有等待该 EventGroup 的任务。

例如：

```text
Task A：等 BIT0
Task B：等 BIT0 | BIT1，任意满足
Task C：等 BIT0 | BIT2，全部满足
Task D：等 BIT0，并 clear-on-exit
```

一个 bit 改变，可能唤醒多个任务。

所以执行时间与 waiter 数量相关。

因此 ISR 版本采用：

```text
ISR
 │
 │ xEventGroupSetBitsFromISR()
 ▼
xTimerPendFunctionCallFromISR()
 │
 ▼
Timer Command Queue
 │
 ▼
Timer / Daemon Task
 │
 ▼
vEventGroupSetBitsCallback()
 │
 ▼
xEventGroupSetBits()
```

也就是说：

> EventGroup FromISR 实际是把操作“投递给 Timer Task 延迟执行”。

---

## 这意味着什么？

它依赖：

```text
configTIMER_QUEUE_LENGTH
configTIMER_TASK_PRIORITY
Timer Task 是否及时得到运行机会
```

如果 Timer Queue 满：

```text
xEventGroupSetBitsFromISR()
        ↓
      pdFAIL
```

如果代码不检查返回值，就可能出现：

```text
ISR 以为事件已经发出
任务一直等不到 bit
系统看起来“偶发卡死”
```

---

## 对象生命周期风险

危险场景：

```text
ISR：
    xEventGroupSetBitsFromISR(event)

Task：
    vEventGroupDelete(event)

之后：
    Timer Task 才执行延迟 callback
```

Timer Task 此时可能访问已经释放的 EventGroup。

> [!important] 工程建议
> 如果是高频 ISR 唤醒单个任务，优先考虑：
>
> ```c
> vTaskNotifyGiveFromISR()
> ```
>
> 通常比 EventGroup 更轻、更直接、更容易分析时序。

---

# 4. Mutex 与优先级继承

## Q4：FreeRTOS 的优先级继承到底怎么实现？

经典场景：

```text
H：高优先级
M：中优先级
L：低优先级

H > M > L
```

执行：

```text
L 拿到 Mutex
M 变为 Ready
H 尝试拿 Mutex，阻塞
```

如果没有优先级继承：

```text
M 不断抢占 L
    ↓
L 无法运行
    ↓
L 无法释放 Mutex
    ↓
H 间接被 M 长时间阻塞
```

这就是优先级反转。

---

## FreeRTOS Mutex 的处理

```text
H 等待 L 持有的 Mutex
        ↓
L 临时继承 H 的优先级
        ↓
L 以高优先级运行
        ↓
尽快释放 Mutex
        ↓
恢复基础优先级
```

TCB 里重要成员：

```c
uxPriority
uxBasePriority
uxMutexesHeld
```

等待 Mutex 时会调用：

```c
xTaskPriorityInherit()
```

如果 Mutex Holder 处于 Ready：

```text
从原优先级 Ready List 移除
    ↓
提高 uxPriority
    ↓
插入新的 Ready List
```

---

## 优先级继承能解决死锁吗？

不能。

例如：

```text
Task A：
    take M1
    wait M2

Task B：
    take M2
    wait M1
```

这是循环等待。

无论优先级多高：

```text
A 等 B
B 等 A
```

都不会自动解除。

---

## FreeRTOS 的 PI 是简化模型

如果一个任务同时持有多个 Mutex：

```text
Task L:
    take M1
    take M2
```

不同高优先级任务分别等待：

```text
H1 → M1
H2 → M2
```

FreeRTOS 不做完整的形式化优先级继承依赖图重新计算。

内部使用：

```c
uxMutexesHeld
```

做简化处理。

所以：

> FreeRTOS 的 Priority Inheritance 足够实用，但不是完整的 Priority Ceiling / 完整继承图算法。

> [!important] 面试记忆
> **Priority Inheritance 只解决优先级反转，不解决死锁。**
>
> FreeRTOS 采用的是简化版优先级继承。

---

# 5. Heap_4 与 Heap_5

## Q5：Heap_4 和 Heap_5 的本质区别是什么？

## Heap_4

Heap_4 管理一块逻辑连续堆：

```c
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
```

空闲块用：

```c
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock;
    size_t xBlockSize;
} BlockLink_t;
```

管理。

核心特征：

```text
first-fit 风格查找
按地址排序的 free list
块分裂
相邻空闲块合并
xBlockSize 高位记录 allocated 标记
```

---

## Heap_4 会不会产生碎片？

会。

虽然它支持相邻 free block 合并，但仍然可能发生外部碎片。

例如：

```text
free 20
used
free 20
used
free 20
```

总 free：

```text
60
```

但申请：

```text
40
```

仍可能失败。

---

## Heap_5

Heap_5 本质上是：

> 在多个不连续物理内存区域上，建立一个统一的 FreeRTOS Heap。

例如：

```c
HeapRegion_t regions[] =
{
    { SRAM1, 32 * 1024 },
    { SRAM2, 64 * 1024 },
    { DDR,    2 * 1024 * 1024 },
    { NULL,   0 }
};

vPortDefineHeapRegions(regions);
```

这些内存区域需要按地址从低到高排列。

---

## 关键纠正：Heap_5 不是 AMP Shared Memory Allocator

错误理解：

```text
A53 + M7 是异构多核
所以共享 DDR 必须用 Heap_5
```

不对。

Heap_5 不负责：

```text
跨核锁
cache coherence
memory barrier
ownership
共享 allocator 元数据同步
DMA cache 属性
```

AMP 场景：

```text
Linux / A53
     │
 Shared DDR
     │
FreeRTOS / M7
```

更合理的共享内存设计是：

```text
固定 buffer pool
descriptor ring
producer index
consumer index
ownership 标记
memory barrier
cache clean / invalidate
doorbell / IPI
```

内存还可以按用途划分：

```text
控制结构 → TCM / SRAM
DMA Buffer → Non-cache SRAM
大块数据 → DDR
```

> [!important] 面试记忆
> **Heap_4：单逻辑堆。**
>
> **Heap_5：多段不连续物理内存组成一个 FreeRTOS 堆。**
>
> **Heap_5 不是 AMP 共享内存同步方案。**

---

# 6. HardFault 与栈溢出调试

## Q6：`pxCurrentTCB` 能不能直接定位 HardFault 是哪个 Task 造成的？

可以辅助判断，但不能作为最终结论。

`pxCurrentTCB` 只能说明：

```text
当前被调度运行的 Task 是谁
```

但 HardFault 可能发生在：

```text
UART ISR
DMA ISR
Timer ISR
其他异常处理
```

此时：

```text
pxCurrentTCB
```

仍然指向被中断的 Task。

所以标准做法不是先看 Task，而是先看：

```text
LR / EXC_RETURN
```

判断异常栈帧位于：

```text
MSP
还是
PSP
```

然后提取：

```text
R0
R1
R2
R3
R12
LR
PC
xPSR
```

其中最重要的是：

```text
stacked PC
```

同时检查：

```text
SCB->CFSR
SCB->HFSR
SCB->MMFAR
SCB->BFAR
```

然后再结合：

```text
pxCurrentTCB
pcTaskName
pxStack
pxTopOfStack
```

---

## `pxTopOfStack` 不是实时 PSP

这是一个非常容易误解的点。

`pxTopOfStack` 是：

> 任务上下文保存时记录下来的 Saved Stack Pointer。

但任务真实执行过程中：

```text
函数调用
局部变量
寄存器 spill
嵌套调用
```

都会持续改变 PSP。

所以：

```text
pxTopOfStack != 当前实时 PSP
```

---

## `configCHECK_FOR_STACK_OVERFLOW = 2`

对于向下增长的栈，FreeRTOS 会检查：

```text
saved stack pointer 是否越界
```

并检查栈底填充 pattern：

```text
0xA5A5A5A5
```

如果 pattern 被破坏：

```text
说明栈区域发生过非法写入
```

但不能百分百证明：

```text
一定是该 Task 自己栈溢出
```

因为别的野指针也可能写坏这个区域。

> [!important] HardFault 排查顺序
>
> ```text
> EXC_RETURN
> → MSP / PSP
> → stacked PC / LR
> → CFSR / HFSR / BFAR / MMFAR
> → pxCurrentTCB
> → 栈 / List / Heap
> ```

---

# 7. 调度器的本质

## Q7：FreeRTOS 调度器本质上在管理什么？

本质就是：

> **TCB 在不同链表之间移动。**

简化 TCB：

```text
TCB
├── pxTopOfStack
├── xStateListItem
├── xEventListItem
├── uxPriority
├── pxStack
├── pcTaskName
├── uxBasePriority
├── uxMutexesHeld
└── Task Notification
```

主要内核链表：

```text
pxReadyTasksLists[priority]

xDelayedTaskList1
xDelayedTaskList2

pxDelayedTaskList
pxOverflowDelayedTaskList

xPendingReadyList

xSuspendedTaskList
```

任务状态可以近似理解为：

> TCB 当前挂在哪条链表上。

```text
                 event / timeout
       ┌──────────────────────────┐
       │                          ▼
 Ready List ◄────────────── Blocked
    │                          │
    │ scheduler                │
    ▼                          │
 Running ─────block────────────┘
    │
    ├────vTaskDelay──────────► Delayed List
    │
    ├────vTaskSuspend────────► Suspended List
    │
    └────vTaskDelete─────────► Termination
```

---

# 8. 为什么阻塞任务需要两个 ListItem

## Q8：为什么一个 Blocked Task 可能同时属于两个逻辑队列？

例如：

```c
xQueueReceive(q, &data, 100);
```

Task 有两个唤醒条件：

```text
1. Queue 收到数据
2. 100 tick 超时
```

所以 TCB 中存在：

```text
xEventListItem
xStateListItem
```

例如：

```text
xEventListItem
    ↓
Queue.xTasksWaitingToReceive

xStateListItem
    ↓
Delayed List
```

如果数据先到：

```text
从 Event List 删除
从 Delayed List 删除
加入 Ready List
```

如果先超时：

```text
从 Delayed List 删除
从 Event List 删除
加入 Ready List
```

这就是 FreeRTOS “事件等待 + 超时等待”可以同时存在的根本原因。

---

# 9. FreeRTOS 调度是否 O(1)

## Q9：能不能说 FreeRTOS 调度器是 O(1)？

不能笼统这样说。

在 Cortex-M 优化端口中：

```c
configUSE_PORT_OPTIMISED_TASK_SELECTION = 1
```

Ready Priority 用 bitmap 管理。

ARM 可以使用：

```text
CLZ
```

快速找到最高 Ready Priority。

所以：

```text
最高优先级选择
```

可以近似看作 O(1)。

但其他内核路径不是。

例如：

```text
vListInsert()             O(N)
EventGroup SetBits        O(N waiters)
xTaskIncrementTick()      O(K 到期任务数)
heap_4 malloc             O(Free Block 数)
Queue copy                O(Item Size)
```

> [!important] 面试回答
> 不要说：
>
> **FreeRTOS 所有调度都是 O(1)。**
>
> 应该说：
>
> **Cortex-M 优化端口的最高 Ready Priority 选择可以是 O(1)，但完整 API 的 WCET 取决于具体内核路径。**

---

# 10. SysTick 到底做了什么

## Q10：SysTick 只是 `tick++` 吗？

不是。

`xTaskIncrementTick()` 可能执行：

```text
xTickCount++
处理 tick wrap
检查 xNextTaskUnblockTime
遍历所有当前 tick 到期任务
从 Delayed List 删除
从 Event List 删除
加入 Ready List
判断是否抢占
判断是否时间片切换
判断是否触发 PendSV
```

例如：

```text
Task A：tick 1000 唤醒
Task B：tick 1000 唤醒
Task C：tick 1000 唤醒
Task D：tick 1000 唤醒
```

那么这个 Tick ISR 的执行时间就会明显变长。

所以：

> **SysTick ISR 的 WCET 与“同一 tick 到期的任务数量”有关。**

这在硬实时分析中非常重要。

---

# 11. Tick 溢出

## Q11：Tick 计数器溢出后，FreeRTOS 为什么还能正常 Delay？

FreeRTOS 使用两个 Delayed List：

```text
Current Delayed List
Overflow Delayed List
```

32 位 tick：

```text
0xFFFFFFFE
0xFFFFFFFF
0x00000000
0x00000001
```

如果某任务的 Wake Time 跨越 wrap：

```text
当前 tick = 0xFFFFFFF0
delay = 100
wake tick = 0x00000054
```

任务会进入 Overflow Delayed List。

当：

```text
xTickCount == 0
```

内核交换：

```text
pxDelayedTaskList
pxOverflowDelayedTaskList
```

因此自然处理 Tick Wrap。

对于自己写的超时代码，通常推荐：

```c
if ((TickType_t)(now - start) >= timeout)
{
    ...
}
```

而不是直接做绝对时间大小判断。

---

# 12. vTaskDelay、xTaskDelayUntil、xTaskPeriodicDelay

## Q12：三者区别是什么？

## `vTaskDelay()`

相对延时：

```text
从现在开始睡 N tick
```

例如：

```text
执行 3ms
delay 10ms
执行 4ms
delay 10ms
```

周期变成：

```text
13ms
14ms
...
```

会累计漂移。

---

## `xTaskDelayUntil()`

基于绝对周期基准：

```text
previousWakeTime + period
```

理想唤醒：

```text
0
10
20
30
40
```

即使任务执行时间有波动，也不会像 `vTaskDelay()` 一样持续累积漂移。

---

## `xTaskPeriodicDelay()`

FreeRTOS V11.3.1 增加的新接口。

它用于改善周期任务发生 overrun 后的行为。

内部会分析：

```text
已经经过了几个周期
```

而不是简单地让 PreviousWakeTime 连续落后。

> [!important] 面试记忆
> `vTaskDelay()`：相对延时。
>
> `xTaskDelayUntil()`：绝对周期基准。
>
> `xTaskPeriodicDelay()`：更适合处理周期任务 overruns 的新接口。

---

# 13. 同优先级任务如何调度

## Q13：两个同优先级 Task 如何轮转？

当：

```c
configUSE_PREEMPTION = 1
configUSE_TIME_SLICING = 1
```

并且同优先级有多个 Ready Task：

```text
Task A
Task B
Task C
```

SysTick 会请求同优先级切换。

形成：

```text
A → B → C → A
```

类似 Round-Robin。

如果：

```c
configUSE_TIME_SLICING = 0
```

Tick 不再强制切换同优先级任务。

但以下事件仍可能导致任务切换：

```text
taskYIELD()
阻塞
删除
优先级变化
中断唤醒其他任务
```

因此：

> 关闭 time slicing 不代表某个 Task 会一直运行到函数退出。

---

# 14. 临界区、挂起调度器、Mutex 的区别

## Q14：这三个机制分别应该用在哪里？

## Critical Section

作用：

```text
保证极短的原子操作
```

Cortex-M BASEPRI Port 下会屏蔽：

```text
RTOS-aware interrupt
```

适合：

```text
极短变量修改
寄存器操作
链表/指针状态更新
```

要求：

> 尽可能短。

---

## `vTaskSuspendAll()`

作用：

```text
停止 Task 调度
但中断仍然允许执行
```

期间 ISR 仍可能发生。

如果 ISR 唤醒任务：

```text
不能立刻正常完成 Ready List 更新
```

FreeRTOS 会使用：

```text
xPendingReadyList
```

等机制延迟处理。

适合：

```text
需要保证调度状态稳定
但又不希望长时间关闭中断
```

---

## Mutex

作用：

```text
Task 与 Task 之间的资源所有权保护
```

特点：

```text
只有竞争资源的 Task 被阻塞
支持 Priority Inheritance
```

典型资源：

```text
SPI
I2C
UART driver
文件系统
共享设备
共享对象
```

> [!important] 记忆
> Critical Section：解决短原子操作。
>
> Scheduler Suspend：阻止任务切换。
>
> Mutex：解决任务间资源所有权。

---

# 15. 为什么 ISR 后必须 portYIELD_FROM_ISR

## Q15：`xQueueSendFromISR()` 已经把高优先级任务唤醒了，为什么还要 Yield？

场景：

```text
Low Task Running
      ↓
UART ISR
      ↓
xQueueSendFromISR()
      ↓
High Task Ready
```

`xQueueSendFromISR()` 只会告诉你：

```c
xHigherPriorityTaskWoken = pdTRUE;
```

它不会在函数内部直接切换 Task。

ISR 末尾需要：

```c
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

Cortex-M 上本质通常是：

```text
Pend PendSV
```

如果漏掉：

```text
High Task 已经 Ready
但是 Low Task 继续执行
直到下一次 Tick / Yield / Block
```

常见现象：

```text
额外多出接近 1 tick 的延迟
```

> [!important] 记忆
> **Task 被唤醒 != Task 立即运行。**
>
> `portYIELD_FROM_ISR()` 的作用是请求 ISR 返回后立即重新调度。

---

# 16. configMAX_SYSCALL_INTERRUPT_PRIORITY

## Q16：为什么这是 Cortex-M FreeRTOS 最容易踩坑的配置之一？

因为存在两套表示：

```text
CMSIS 逻辑优先级
NVIC 寄存器编码
```

例如实现 4 个 Priority Bits：

```text
CMSIS priority = 5
```

实际 NVIC Priority Register 中：

```text
0x50
```

典型配置：

```c
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (5 << (8 - configPRIO_BITS))
```

危险代码：

```c
NVIC_SetPriority(UART_IRQn, 3);

void UART_IRQHandler(void)
{
    xQueueSendFromISR(...);
}
```

Priority 3 比 5 更高。

意味着：

```text
该中断不会被 FreeRTOS BASEPRI 临界区屏蔽
```

但它却进入了 Kernel API。

这是非法配置。

---

## 另一个经典坑

很多 MCU 中断默认优先级：

```text
0
```

这是最高优先级。

所以如果你没有显式设置优先级：

```c
HAL_NVIC_SetPriority(...)
```

ISR 又调用了 FreeRTOS API，就很危险。

开发阶段务必打开：

```c
configASSERT()
```

---

# 17. 零延迟中断应该如何设计

## Q17：特别高优先级的 ISR 如何把数据交给 FreeRTOS Task？

不要直接调用 FreeRTOS。

可以做两级中断：

```text
ADC High Priority IRQ
priority = 2
       │
       ├─ 清中断
       ├─ 记录数据
       ├─ 记录时间戳
       └─ Pend 一个低优先级 SW IRQ
                         │
                         ▼
                  priority = 6
                         │
                         └─ vTaskNotifyGiveFromISR()
                                  │
                                  ▼
                               Worker Task
```

这是：

```text
高优先级 Top Half
+
RTOS-aware Bottom Half
```

如果硬件延迟要求没那么严：

> 最简单的方案还是直接把硬件 ISR 配置到可调用 FreeRTOS API 的优先级范围。

---

# 18. 为什么 Task Notification 很快

## Q18：为什么 ISR → Task 通知优先考虑 Task Notification？

Task Notification 数据直接放在 TCB 里：

```c
ulNotifiedValue[]
ucNotifyState[]
```

因此：

```text
ISR
 ↓
vTaskNotifyGiveFromISR(TaskA)
 ↓
直接修改 TaskA TCB
```

不需要额外创建：

```text
Queue Object
Semaphore Object
EventGroup Object
```

它可以模拟：

```text
Binary Semaphore
Counting Semaphore
Event Bits
32 位消息值
```

适合：

```text
DMA IRQ → Worker Task
UART IRQ → RX Task
ADC IRQ → Processing Task
```

限制：

```text
Notification 属于某个具体 Task
```

所以不适合做：

```text
多消费者广播
资源所有权
Priority Inheritance
```

---

# 19. Binary Semaphore 与 Mutex

## Q19：Binary Semaphore 和 Mutex 有什么区别？

底层都和 Queue 体系相关，但语义不同。

## Binary Semaphore

表示：

```text
事件 / Token
```

典型：

```text
ISR Give
Task Take
```

没有严格 owner 概念。

---

## Mutex

表示：

```text
资源所有权
```

典型：

```text
Task A take
Task A 操作资源
Task A give
```

内核会记录：

```text
Mutex Holder
```

并用于：

```text
Priority Inheritance
```

Mutex 不应在 ISR 中使用。

---

## 快速选择

```text
ISR → 单 Task           Task Notification
ISR → 通用事件对象       Binary Semaphore
Task → Task 共享资源     Mutex
```

---

# 20. 优先级反转、饥饿、死锁

## Q20：三者本质区别是什么？

## 优先级反转

```text
High 等 Low 持有的资源
Medium 一直抢占 Low
```

导致：

```text
High 实际间接被 Medium 阻塞
```

Mutex Priority Inheritance 可以缓解。

---

## 饥饿 Starvation

```text
Low 一直 Ready
但是更高优先级 Task 始终 Ready
```

所以 Low 长时间无法获得 CPU。

这是：

```text
优先级设计 / 工作负载设计问题
```

---

## 死锁 Deadlock

```text
A 持有 X，等待 Y
B 持有 Y，等待 X
```

形成：

```text
循环等待
```

Priority Inheritance 不能解决。

常见预防：

```text
统一锁顺序
M1 → M2 → M3
```

---

# 21. EventGroup 常见故障

## Q21：EventGroup 最重要的三个坑是什么？

### 1. ISR SetBits 不是立即执行

```text
ISR
 ↓
Timer Queue
 ↓
Timer Task
 ↓
真正 SetBits
```

---

### 2. Timer Queue 会满

必须检查：

```c
BaseType_t ret =
    xEventGroupSetBitsFromISR(...);
```

如果返回：

```text
pdFAIL
```

说明 deferred command 没进队。

---

### 3. 对象生命周期

危险：

```text
ISR 已经 Pend SetBits
Task 删除 EventGroup
Timer Task 后执行 callback
```

可能形成 Use-After-Free。

---

# 22. 软件定时器与 Timer Daemon Task

## Q22：为什么一个 Timer Callback 写坏，可能拖死很多功能？

软件定时器不是每个 Timer 对应一个 Task。

而是：

```text
Timer A Callback
Timer B Callback
Pended Function
EventGroup Deferred Operation
        │
        ▼
   Timer Service Task
```

都是同一个 Timer/Daemon Task 执行。

所以：

```c
TimerCallback()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
}
```

一旦阻塞：

```text
Timer Task 阻塞
    ↓
其他 Timer Callback 延迟
    ↓
Pended Function 延迟
    ↓
EventGroup ISR deferred 操作延迟
    ↓
Timer Queue 逐渐堆积
```

正确设计：

```text
Timer Callback
    ↓
Notify Worker
    ↓
立即返回

Worker Task
    ↓
做复杂/阻塞操作
```

---

# 23. Queue 的数据复制语义

## Q23：FreeRTOS Queue 里面存的是对象还是指针？

Queue 存储的是：

```text
uxItemSize 字节
```

例如：

```c
xQueueCreate(10, sizeof(MyMessage));
```

会复制整个结构体。

而：

```c
xQueueCreate(10, sizeof(MyMessage *));
```

只复制指针值。

不会复制指针指向的数据。

危险例子：

```c
void foo(void)
{
    uint8_t buf[100];
    uint8_t *p = buf;

    xQueueSend(q, &p, 0);
}
```

`foo()` 返回后：

```text
p 指向的栈内存已经失效
```

所以 Pointer Queue 必须明确：

```text
谁分配
谁拥有
谁释放
什么时候 ownership 转移
```

高吞吐场景常用：

```text
Fixed Buffer Pool
+
Pointer / Descriptor Queue
```

避免复制大块数据。

---

# 24. StreamBuffer 与 MessageBuffer

## Q24：为什么 StreamBuffer 很高效？

它的设计假设：

```text
Single Writer
Single Reader
```

因此能减少多生产者/多消费者同步成本。

典型：

```text
UART ISR
   ↓
StreamBuffer
   ↓
Parser Task
```

MessageBuffer 在 StreamBuffer 基础上保留：

```text
Message Boundary
```

即：

```text
Length + Payload
```

而 StreamBuffer 是纯字节流。

如果存在：

```text
Multiple Writer
Multiple Reader
```

应用层需要自己增加序列化。

---

# 25. 动态内存与实时系统

## Q25：实时系统是不是绝对不能用 Heap_4？

不是。

真正的问题是：

```text
最坏执行时间是否可接受？
失败模式是否可控制？
```

Heap_4 malloc 会扫描 free list。

所以执行时间依赖：

```text
当前堆碎片状态
free block 数量
```

很多实时系统采用：

```text
启动阶段：
    创建 Task
    创建 Queue
    创建 Buffer

运行阶段：
    不再 malloc/free
```

或者直接：

```c
xTaskCreateStatic()
xQueueCreateStatic()
xSemaphoreCreateBinaryStatic()
```

静态分配的优势：

```text
确定性更好
内存预算明确
失败路径更容易控制
```

更准确的说法是：

> 动态分配不是实时系统禁区，但不受控的运行期动态内存不适合关键实时路径。

---

# 26. 任务栈大小

## Q26：为什么 FreeRTOS 的 Task Stack Size 很容易理解错？

Cortex-M4F：

```c
typedef uint32_t StackType_t;
```

所以很多 API 的 Stack Depth 单位不是字节，而是：

```text
StackType_t
```

例如：

```c
xTaskCreate(..., 512, ...);
```

通常代表：

```text
512 words × 4 bytes = 2048 bytes
```

Task Stack 里会消耗：

```text
局部变量
函数嵌套
寄存器保存
Exception Frame
编译器 Spill
printf
浮点计算
C++ Runtime
TLS/newlib 数据
```

M4F 如果任务使用 FPU，上下文占用还会进一步上升。

---

# 27. Stack High Water Mark

## Q27：High Water Mark 能不能证明 Stack 一定够？

不能。

High Water Mark 只是：

> 当前已经执行过的路径中，曾经剩余的最小 Stack。

如果没走过这些路径：

```text
异常流程
错误处理
深层 callback
TLS 握手
printf("%f")
文件系统故障流程
递归
```

就无法证明 Worst Case。

更可靠的方法：

```text
Compiler Stack Usage
静态 Call Chain 分析
压力测试
High Water Mark
额外 Safety Margin
```

不要简单做：

```text
测出来还剩 500 字节
那就把栈再减 400
```

---

# 28. Cortex-M HardFault 标准排查流程

## Q28：推荐的 HardFault 排查流程是什么？

### Step 1：判断异常栈帧来自 MSP 还是 PSP

看：

```text
LR / EXC_RETURN
```

---

### Step 2：提取异常栈帧

读取：

```text
R0
R1
R2
R3
R12
LR
PC
xPSR
```

最关键：

```text
stacked PC
```

---

### Step 3：读取 SCB Fault Register

```text
CFSR
HFSR
BFAR
MMFAR
```

---

### Step 4：把 PC 转成源码位置

```bash
arm-none-eabi-addr2line -e app.elf -f -C <pc>
```

---

### Step 5：结合 FreeRTOS Task 信息

查看：

```text
pxCurrentTCB
pcTaskName
pxStack
pxTopOfStack
```

---

### Step 6：检查 RTOS 高频错误

```text
Stack Overflow
IRQ Priority Error
FromISR API 用错
对象已经删除
List Corruption
Heap Corruption
Scheduler Suspend 时调用阻塞 API
```

---

### Step 7：抓第一次非法写

如果发现：

```text
TCB / List 指针已被破坏
```

可以使用：

```text
Cortex-M DWT Watchpoint
```

监控被破坏地址。

这样可以直接抓到：

```text
第一条写坏内存的指令
```

而不是只看到最终 HardFault 的地方。

> [!important]
> 最后 HardFault 的函数经常只是“受害者”。
>
> 真正的 Bug 往往发生在很早之前。

---

# 29. vTaskDelete 与内存回收

## Q29：为什么 `vTaskDelete()` 后内存不一定立即释放？

如果 Task 删除自己：

```text
Task 当前还在使用自己的 Stack
```

此时不能立刻释放。

所以会先进入：

```text
xTasksWaitingTermination
```

之后由 Idle Task 清理。

如果是其他 Task 删除一个当前没有运行的 Task，内核可能可以更直接清理。

---

## `vTaskDelete()` 会清什么？

如果 Task 是动态创建的：

```text
TCB
Task Stack
```

会由 FreeRTOS 负责回收。

但不会自动释放：

```text
pvPortMalloc Buffer
malloc Buffer
Socket
File
Device Handle
用户自定义内存池
```

所以 Task 生命周期必须有明确资源管理策略。

---

# 30. 为什么“内核崩溃”往往是应用层错误

## Q30：为什么很多 HardFault 最后出现在 `list.c`？

典型过程：

```text
Task Stack Overflow
      ↓
写坏 ListItem
      ↓
系统继续运行
      ↓
某次 ISR 唤醒 Task
      ↓
uxListRemove()
      ↓
访问损坏 pxNext
      ↓
HardFault
```

你看到：

```text
崩在 list.c
```

但真正 Bug 是：

```text
之前的栈溢出
```

另一个例子：

```text
priority 0 ISR
      ↓
xQueueSendFromISR()
      ↓
在 Kernel Critical Section 中强行进入内核
      ↓
List 状态损坏
      ↓
随机时间后崩溃
```

开发建议：

```c
configASSERT(...)
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
```

不要为了“让系统继续跑”而关闭 assert。

---

# 31. SMP 与 AMP

## Q31：FreeRTOS SMP 和 AMP 的本质区别是什么？

## SMP

```text
        一个 FreeRTOS Kernel
              │
       Shared Scheduler
          /        \
       Core0      Core1
```

共享：

```text
Ready List
Task State
Scheduler State
Kernel Lock
```

可能有：

```c
configNUMBER_OF_CORES
pxCurrentTCBs[]
```

---

## AMP

```text
Linux / A53
   Kernel
     │
     │ IPC
     ▼
FreeRTOS / M7
   Kernel
```

双方完全独立：

```text
独立 Scheduler
独立 TCB
独立 Heap
独立 IRQ
独立 OS 生命周期
```

通过：

```text
Shared Memory
Mailbox
IPI
RPMsg
IPC-SHM
```

通信。

> [!important]
> **SMP = 一个 OS 管多个核。**
>
> **AMP = 多个 OS / Kernel 分别管理各自核心，再通过 IPC 通信。**

---

# 32. FreeRTOS + newlib

## Q32：`configUSE_NEWLIB_REENTRANT=1` 是否意味着 libc 全部线程安全？

不是。

开启后，FreeRTOS 可以为每个 Task 提供自己的：

```c
struct _reent
```

并在任务切换时切换相应 newlib reentrant state。

可以改善：

```text
errno
部分 stdio 状态
部分 libc reentrant 状态
```

但不代表：

```text
所有 libc 全局资源自动线程安全
```

例如 newlib 的：

```text
malloc/free
```

仍然可能需要：

```text
__malloc_lock()
__malloc_unlock()
```

另外：

```c
printf()
```

最终可能走：

```text
_write()
UART
ITM
Semihosting
```

这些底层输出资源本身也需要同步。

更合理的 Logging 架构：

```text
Task A ─┐
Task B ─┼─ Log Queue → Logger Task → UART
Task C ─┘
```

---

# 33. FreeRTOS + lwIP

## Q33：为什么 lwIP 在 FreeRTOS 下经常“偶发崩溃”？

先理解 lwIP 的线程模型。

典型 OS 模式：

```text
Application Task
      │
      ▼
tcpip_thread
      │
      ▼
lwIP Core
```

lwIP Raw API 一般要求：

```text
在 tcpip_thread 上下文调用
```

或者开启并正确使用：

```text
Core Locking
```

危险：

```c
Task A:
    tcp_write(...);

Task B:
    tcp_close(...);
```

如果没有正确序列化，可能破坏同一个 PCB。

更危险：

```c
ETH_IRQHandler()
{
    udp_send(...);
}
```

ISR 直接调 Raw API 通常不对。

正确思路：

```text
ETH DMA IRQ
    ↓
Driver / Network Task
    ↓
tcpip_input()
    ↓
tcpip_thread
```

对于 M7/H7 这类带 D-Cache 的 SoC，还必须额外检查：

```text
DMA Buffer
Cache Line Alignment
Cache Clean
Cache Invalidate
Zero-Copy Ownership
```

很多所谓“lwIP 问题”其实是：

```text
Cache Coherence
Buffer Ownership
```

问题。

---

# 34. FreeRTOS + FatFs

## Q34：FatFs 为什么在多任务下可能把 SD 卡文件系统写坏？

FatFs 可以配置：

```c
FF_FS_REENTRANT = 1
```

并提供 Mutex Hook。

这只能保证：

```text
FatFs 自己的文件系统对象
```

在一定条件下受保护。

但并不自动保证：

```text
SDMMC Driver
SPI Driver
DMA Buffer
disk_read()
disk_write()
disk_ioctl()
```

线程安全。

更简单的工程架构：

```text
             FileSystem Task
                  ▲
                  │
           File Request Queue
       ┌──────────┼──────────┐
     TaskA       TaskB      TaskC
```

所有文件系统操作统一在一个 Task 中做。

这样可以大幅减少：

```text
锁
Driver 重入
DMA 冲突
对象生命周期
```

问题。

---

# 35. 实时性的本质

## Q35：实时是不是等于“运行得很快”？

不是。

实时系统最核心的是：

```text
Timing Predictability
Bounded Latency
Deadline Guarantee
```

例如：

```text
System A：
平均 5 us
最坏 500 ms

System B：
稳定 100 us ± 5 us
```

对于硬实时系统：

```text
System B
```

可能更优秀。

真正应该分析：

```text
Interrupt Latency
Scheduler Latency
Context Switch WCET
Critical Section WCET
Blocking Time
Priority Inversion
ISR Burst
Timer Callback WCET
Heap WCET
Task Response Time
Deadline
```

而不是只看：

```text
CPU 占用率低不低
平均速度快不快
```

---

# 36. 为什么 ISR 必须短

## Q36：为什么中断函数一定要尽量短？

因为 ISR 本身就是对：

```text
Task
低优先级 ISR
```

的干扰。

如果一个 ISR 执行：

```text
100 us
```

那么高优先级 Task 即使已经 Ready，也要先等这个 ISR 结束。

典型 RTOS 架构：

```text
ISR / Top Half
    │
    ├─ 清中断
    ├─ 取最少量数据
    ├─ 记录 timestamp
    └─ Notification / Queue
                │
                ▼
          Worker Task
                │
                └─ 协议处理 / 算法 / IO
```

`FromISR` API 的根本价值之一，就是：

> 让 ISR 快速把工作转移到 Task Context。

---

# 37. 为什么 volatile 不能代替同步机制

## Q37：为什么 `volatile` 不能解决多任务并发？

例如：

```c
volatile uint32_t count;
count++;
```

看起来只有一行。

但 CPU 实际可能是：

```text
load
add
store
```

如果：

```text
Task load 5
ISR  load 5
ISR  store 6
Task store 6
```

最终还是：

```text
6
```

而不是：

```text
7
```

丢了一次更新。

`volatile` 主要告诉编译器：

```text
这个变量每次都要真正访问内存
不要随意优化掉
```

但它不提供：

```text
Mutual Exclusion
Atomic Read-Modify-Write
Memory Ordering
Cross-Core Visibility
Ownership
```

真正需要的是：

```text
Critical Section
Atomic Operation
Mutex
Semaphore
Memory Barrier
```

取决于具体问题。

---

# 38. 为什么 FromISR API 不能阻塞

## Q38：为什么 `xQueueSendFromISR()` Queue 满了只能失败，不能等？

Task 可以 Block，是因为 Task 有：

```text
TCB
Task State
Ready / Blocked / Delayed
```

例如：

```text
Task
 ↓
xQueueSend(..., 100)
 ↓ Queue Full
TCB → Blocked
 ↓
Scheduler 切其他 Task
```

ISR 不一样。

ISR 是：

```text
CPU Exception Context
```

它没有：

```text
ISR TCB
ISR Ready State
ISR Blocked State
```

所以 ISR API 必须：

```text
立即成功
或者
立即失败
```

不能：

```text
Sleep 等资源
```

---

# 39. 为什么 FreeRTOS 大量使用链表

## Q39：为什么内核不用数组，反而大量使用双向链表？

因为内核经常做：

```text
插入一个等待 Task
删除任意 Task
Timeout 一个 Task
按优先级找到 waiter
在多个状态之间移动 Task
```

双向链表对已知节点删除很方便：

```text
pxPrevious->pxNext
pxNext->pxPrevious
```

代价是：

```text
指针一旦被破坏，后果非常严重
```

例如：

```text
pxNext
pxPrevious
pxContainer
```

任意一个被野指针写坏，后面一次普通 `uxListRemove()` 都可能 HardFault。

所以：

```text
List Crash
```

经常不是 List Algorithm Bug，而是：

```text
Stack Overflow
Buffer Overflow
Bad IRQ Priority
Use-After-Free
```

导致的后果。

---

# 40. 为什么调度不依赖 SysTick

## Q40：FreeRTOS 是不是每个 Tick 才切一次任务？

不是。

任何让：

```text
Runnable Set
```

发生变化的事件，都可能触发调度：

```text
DMA ISR Post Queue
UART ISR Give Notification
Task Give Semaphore
Task Block
Task Yield
Task Delete
Task Change Priority
```

抽象模型：

```text
Runnable Set 改变
        ↓
检查最高优先级 Ready Task
        ↓
必要时 Pend PendSV
```

所以 SysTick 只是：

> 触发调度的一种来源。

在配置正确的抢占式系统中：

```text
ISR 唤醒高优先级 Task
    ↓
ISR Return
    ↓
PendSV
    ↓
立即运行高优先级 Task
```

不需要等下一个 Tick。

---

# 41. 面试优先级总结

## S 级：必须讲到底层

- Task / TCB / Stack
- Ready List / Delayed List / Event List
- SysTick / PendSV / SVC
- PSP / MSP
- EXC_RETURN
- Context Save / Restore
- BASEPRI
- `configMAX_SYSCALL_INTERRUPT_PRIORITY`
- Normal API vs FromISR API
- `portYIELD_FROM_ISR`

---

## S 级：同步机制

- Queue
- Binary Semaphore
- Mutex
- Task Notification
- Priority Inversion
- Priority Inheritance
- Deadlock

---

## A 级：内核机制

- EventGroup Deferred ISR
- Timer Queue
- Timer Daemon Task
- Tick Overflow
- `vTaskDelay`
- `xTaskDelayUntil`
- `xTaskPeriodicDelay`
- Heap_4
- Heap_5
- Static Allocation
- Stack Overflow
- HardFault

---

## A 级：工程调试

- `configASSERT`
- `configCHECK_FOR_STACK_OVERFLOW`
- malloc failed hook
- `pxCurrentTCB`
- Exception Stack Frame
- CFSR
- HFSR
- BFAR
- MMFAR
- DWT Watchpoint

---

## B+ 级：三方组件

- StreamBuffer
- MessageBuffer
- FreeRTOS SMP
- AMP
- newlib Reentrancy
- lwIP Thread Model
- FatFs Reentrancy

---

# 42. 最终记忆笔记

> [!summary] FreeRTOS 一段话心智模型
> FreeRTOS 本质上是一个**基于优先级的任务状态机**。任务由 TCB 和独立 Stack 构成，任务是否可运行取决于 TCB 当前位于 Ready、Delayed、Event、Suspended 等哪类链表。Cortex-M 上，SysTick 主要负责时间推进和任务唤醒，PendSV 负责上下文保存与恢复，SVC 用于启动首任务等特权切换。`BASEPRI` 划定了 RTOS-aware ISR 和 Zero-Latency ISR 的边界。Queue、Semaphore、Mutex、Notification、EventGroup、Timer 的本质差异，在于**所有权、等待者数量、是否延迟执行、是否复制数据、是否支持优先级继承，以及最坏执行时间是否可控**。

> [!summary] Kernel Crash 快速排查
> 如果 FreeRTOS “随机崩在内核”，优先排查：
>
> 1. Stack Overflow  
> 2. IRQ Priority 错误  
> 3. ISR / Task API 用错  
> 4. Object Lifetime  
> 5. Buffer Overflow  
> 6. Heap / List Corruption  
> 7. Timer Task 阻塞  
>
> **最终 Crash 位置经常不是 Bug 首发位置。**

> [!summary] 面试回答的最高层次
> 一个成熟的 FreeRTOS 回答要区分四层：
>
> ```text
> ARM 架构机制
>     ↓
> FreeRTOS Port 实现
>     ↓
> FreeRTOS Kernel 策略
>     ↓
> Application 工程设计
> ```
>
> 例如：
>
> ```text
> BASEPRI           → ARM 架构
> xPortPendSVHandler → Port
> Ready List         → Kernel
> UART IRQ → Notify  → Application
> ```
>
> 不要把四层概念混在一起。

---

# 参考资料

## FreeRTOS 官方仓库

- FreeRTOS 主仓库  
  https://github.com/FreeRTOS/FreeRTOS

- FreeRTOS Kernel  
  https://github.com/FreeRTOS/FreeRTOS-Kernel

- Cortex-M4F GCC Port  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/main/portable/GCC/ARM_CM4F

- `tasks.c`  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/tasks.c

- `queue.c`  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/queue.c

- `event_groups.c`  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/event_groups.c

- `timers.c`  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/timers.c

- `list.c`  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/list.c

- Heap_4  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/MemMang/heap_4.c

- Heap_5  
  https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/MemMang/heap_5.c

---

## FreeRTOS 官方文档

- Cortex-M Interrupt Priority  
  https://www.freertos.org/RTOS-Cortex-M3-M4.html

- Mutex  
  https://www.freertos.org/Real-time-embedded-RTOS-mutexes.html

- Stack Overflow Checking  
  https://www.freertos.org/Stacks-and-stack-overflow-checking.html

- Software Timer  
  https://www.freertos.org/RTOS-software-timer.html

- Task Notification  
  https://www.freertos.org/RTOS-task-notifications.html

---

## ARM

- Cortex-M Interrupt Priority  
  https://developer.arm.com/community/arm-community-blogs/b/embedded-and-microcontrollers-blog/posts/cutting-through-the-confusion-with-arm-cortex-m-interrupt-priorities

---

## 第三方组件

- lwIP Multithreading  
  https://www.nongnu.org/lwip/2_1_x/multithreading.html

- lwIP Pitfalls  
  https://www.nongnu.org/lwip/2_1_x/pitfalls.html

- FatFs Application Note  
  https://elm-chan.org/fsw/ff/doc/appnote.html

- newlib libc  
  https://sourceware.org/newlib/libc.html
