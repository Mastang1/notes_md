# FreeRTOS：TCB、5种 Heap 与任务通信机制

> 基准源码：`FreeRTOS/FreeRTOS` 的 `main` 当前把 `FreeRTOS/Source` 固定到 `FreeRTOS-Kernel` 提交 `3a22924e0a9ddbbc8b0758881c33b3422a5cc20d`，对应 FreeRTOS Kernel V11.3.1。
>
> 本文只讲三个问题：**TCB 里到底存什么、5 个 heap 到底怎么管内存、任务之间到底有哪些通信/同步方式。**

## 1. TCB：一个任务在内核里的“档案袋”

费曼理解：CPU 真正运行任务时只认识“寄存器 + 栈”；调度器还要知道它**在哪个状态链表、优先级多少、在等什么事件、有没有互斥锁、有没有通知**。所以 TCB 就是把“CPU 上下文索引 + 调度状态 + IPC 状态”装在一起的任务档案。

下面完整列出该版本 `tasks.c` 中的 `tskTaskControlBlock`，并给每个有效字段加注释：

```c
typedef struct tskTaskControlBlock
{
    volatile StackType_t * pxTopOfStack;      // 当前任务保存上下文后的栈顶；必须是 TCB 第1成员，端口汇编切换上下文直接依赖它。

#if ( portUSING_MPU_WRAPPERS == 1 )
    xMPU_SETTINGS xMPUSettings;               // MPU 区域/权限配置；启用 MPU 时必须是第2成员。
#endif

#if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 )
    UBaseType_t uxCoreAffinityMask;            // SMP 核亲和性掩码：该任务允许在哪些 CPU Core 上运行。
#endif

    ListItem_t xStateListItem;                 // 调度状态链表节点：Ready/Blocked/Suspended 等状态靠它挂链。
    ListItem_t xEventListItem;                 // 事件等待链表节点：任务等待 Queue/Semaphore/Event 等对象时挂到对象等待链表。
    UBaseType_t uxPriority;                    // 当前有效优先级，0 最低；可能因 Mutex 优先级继承临时升高。
    StackType_t * pxStack;                     // 任务栈起始地址，用于栈管理、检查和释放。

#if ( configNUMBER_OF_CORES > 1 )
    volatile BaseType_t xTaskRunState;         // SMP：记录当前运行在哪个 Core，或“不运行/待让出”等内部状态。
    UBaseType_t uxTaskAttributes;              // SMP 任务属性，目前主要标识 Idle Task。
#endif

    char pcTaskName[ configMAX_TASK_NAME_LEN ]; // 任务名；主要用于调试、Trace、Kernel-aware debugger。

#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
    BaseType_t xPreemptionDisable;             // 任务级禁止抢占状态。
#endif

#if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
    StackType_t * pxEndOfStack;                // 栈最高有效地址，用于不同栈增长方向/栈边界检查。
#endif

#if ( portCRITICAL_NESTING_IN_TCB == 1 )
    UBaseType_t uxCriticalNesting;             // 临界区嵌套深度；某些 port 把嵌套计数保存在 TCB 中。
#endif

#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxTCBNumber;                   // TCB 创建序号，帮助调试器识别“删除后又创建”的任务。
    UBaseType_t uxTaskNumber;                  // 提供给第三方 Trace 工具使用的任务编号。
#endif

#if ( configUSE_MUTEXES == 1 )
    UBaseType_t uxBasePriority;                // 基础优先级；Mutex 优先级继承结束后恢复到这里。
    UBaseType_t uxMutexesHeld;                 // 当前持有 Mutex 数量；参与优先级继承/反继承处理。
#endif

#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    TaskHookFunction_t pxTaskTag;              // 应用自定义 Task Hook/Tag。
#endif

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
    void * pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ]; // FreeRTOS 提供的任务私有 TLS 指针数组。
#endif

#if ( configGENERATE_RUN_TIME_STATS == 1 )
    configRUN_TIME_COUNTER_TYPE ulRunTimeCounter; // 累计任务处于 Running 状态的运行时间统计值。
#endif

#if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
    configTLS_BLOCK_TYPE xTLSBlock;            // C Runtime 自己的 TLS 数据块，例如 newlib/工具链线程局部状态。
#endif

#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ]; // 每个通知槽保存一个 32-bit 值。
    volatile uint8_t ucNotifyState[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];    // 每个通知槽的等待/已通知状态。
#endif

#if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
    uint8_t ucStaticallyAllocated;             // 标记任务对象是否静态分配，删除时决定是否真正 free。
#endif

#if ( INCLUDE_xTaskAbortDelay == 1 )
    uint8_t ucDelayAborted;                    // 标记阻塞/延时是否被 xTaskAbortDelay() 提前终止。
#endif

#if ( configUSE_POSIX_ERRNO == 1 )
    int iTaskErrno;                            // 每任务 errno；切换任务时同步到 FreeRTOS_errno。
#endif
} tskTCB;

typedef tskTCB TCB_t;                         // 新名字；保留旧 tskTCB 是为了兼容 Kernel-aware debugger。
```

### TCB 模型

把 TCB 压缩成 5 块就够了：

`TCB = CPU上下文入口 + 调度状态 + 优先级/互斥状态 + 任务私有数据 + IPC等待状态`

其中最关键的两个“挂钩”是：

- `xStateListItem`：回答“**任务现在处于什么调度状态？**”
- `xEventListItem`：回答“**任务现在在等哪个内核对象？**”

而 `pxTopOfStack` 是调度器最终把软件状态重新变成 CPU 执行现场的入口。

---

## 2. 5 个 Heap：本质都是在回答“空闲内存怎么记、怎么找、怎么回收”

### heap_1：只分配，不回收

费曼比喻：一卷纸只能不断往后撕，撕出去的纸绝不粘回来。

机制：一个 `ucHeap[]` 加一个 `xNextFreeByte` 游标；`pvPortMalloc()` 对齐后直接返回当前游标，再把游标向后移动；`vPortFree()` 实际禁止使用。

适合：系统启动阶段创建完对象后永不删除，行为最简单、最容易推理。

核心模型：

`heap_1 = 线性指针 + bump allocation + no free`

### heap_2：能回收，但不合并

费曼比喻：退回来的箱子按“箱子大小”重新摆到货架上，但两个挨着的小箱子永远不会拆墙合成大箱子。

机制：每个空闲块前面有 `BlockLink_t { next, size }`；空闲链表按**块大小**排序，申请时从小到大找到第一个足够大的块，因此近似 best-fit；大块会切分；释放后重新插回链表，但**不合并物理相邻块**。

结果：总空闲字节可能很多，但被切成很多小洞，最终申请大块仍会失败。

核心模型：

`heap_2 = size-ordered free list + split + free + no coalescing`

### heap_3：把内存管理外包给 C 库

费曼比喻：FreeRTOS 不经营仓库，只负责把申请单交给 libc 的 `malloc/free`。

机制：`pvPortMalloc()` -> `malloc()`，`vPortFree()` -> `free()`；FreeRTOS 用 `vTaskSuspendAll()/xTaskResumeAll()` 包围调用以避免任务并发进入这段路径。真正的碎片策略、元数据、堆区来源都由 C 库/链接器决定。

核心模型：

`heap_3 = FreeRTOS API wrapper + libc allocator`

### heap_4：单连续堆，支持相邻块合并

费曼比喻：仓库中的空房间按“门牌地址”排序；退房时如果左边、右边也是空房，就把墙拆掉，恢复成一个大房间。

机制：空闲链表按**内存地址**排序；申请时从低地址向后找第一个足够大的块（first-fit），必要时切分；释放时插回地址有序链表，并检查：

`前块末地址 == 当前块首地址` → 向前合并；

`当前块末地址 == 后块首地址` → 向后合并。

这就是 heap_4 比 heap_2 更抗外部碎片的根本原因。当前版本还支持 heap protector/canary 和 `vPortGetHeapStats()`。

核心模型：

`heap_4 = address-ordered free list + first-fit + split + bidirectional coalescing`

### heap_5：heap_4 + 多段不连续内存

费曼比喻：现在不是一个仓库，而是片上 SRAM、外部 SRAM、另一段 RAM 多个仓库；FreeRTOS 把它们登记成一个“逻辑库存系统”。

机制：应用先调用 `vPortDefineHeapRegions()` 注册按地址递增排列的 `HeapRegion_t[]`；每段区域内部仍按 heap_4 的地址链表、切分、回收、相邻合并规则工作；不同物理区域之间有空洞，因此不能跨空洞合并。

必须注意：**第一次 `pvPortMalloc()` 前就要先定义区域。**

核心模型：

`heap_5 = heap_4 algorithm + multiple non-contiguous regions`

### 5 个 Heap 一张表

| 模块 | 内存来源 | 可 free | 空闲块组织 | 相邻合并 | 典型选择 |
|---|---|---:|---|---:|---|
| heap_1 | `ucHeap[]` | 否 | 单游标 | 否 | 对象只创建不删除 |
| heap_2 | `ucHeap[]` | 是 | 按大小排序链表 | 否 | 历史方案，允许 free 但接受碎片 |
| heap_3 | C library heap | 是 | 由 libc 决定 | 由 libc 决定 | 必须沿用工具链 malloc/free |
| heap_4 | 单段 `ucHeap[]` | 是 | 按地址排序链表 | 是 | **大多数 MCU 动态分配的默认优先选择** |
| heap_5 | 多段 HeapRegion | 是 | 跨区域地址链表 | 同一区域内可合并 | RAM 不连续/多 SRAM 区域 |

### Heap 的通用设计模型

以后看任何 allocator，只问 6 个问题：

`内存从哪来 → 空闲块怎么记录 → 申请时怎么选块 → 大块是否切分 → free 后是否合并 → 是否支持多内存区域`

FreeRTOS 5 个 heap 的差异，本质全部落在这 6 个问题上。

---

## 3. FreeRTOS 任务通信/同步机制

这里把“任务通信”按内核提供的 IPC/同步原语统计。`vTaskSuspend()/Resume()` 是任务控制，不算正式 IPC；Software Timer 是定时执行机制，也不单列为任务通信通道。

| 机制 | 传什么 | 典型方向 | 阻塞能力 | 底层核心 | 最适合 |
|---|---|---|---|---|---|
| Queue | 固定长度数据项 | 多生产者/多消费者 | 发送/接收均可阻塞 | `Queue_t + 数据区 + 两个等待链表` | 结构体、命令、事件消息 |
| Binary Semaphore | 0/1 事件 | Task↔Task、ISR→Task | Take 可阻塞 | **Queue，长度1、item size=0** | “发生了一次事件” |
| Counting Semaphore | 计数值 | 多方 | Take 可阻塞 | **Queue，item size=0，消息数当计数** | 资源计数、累计事件 |
| Mutex | 所有权 | Task↔Task | Lock 可阻塞 | **特殊 Queue + owner + 优先级继承** | 保护共享资源 |
| Recursive Mutex | 所有权+递归次数 | Task↔Task | Lock 可阻塞 | Mutex + recursion count | 同一任务嵌套加锁 |
| Task Notification | 32-bit 值/bit/count/纯事件 | 指定 Task | Wait/Take 可阻塞 | **直接存在目标任务 TCB 内** | 最轻量 ISR→Task、Task→Task 通知 |
| Event Group | 多 bit 条件 | 多任务 | Wait any/all 可阻塞 | `EventBits_t + 等待任务链表` | 多条件状态、阶段同步 |
| Stream Buffer | 连续字节流 | 默认单写者/单读者 | Send/Receive 可阻塞 | **ring buffer + Task Notification 唤醒** | UART/TCP-like byte stream |
| Message Buffer | 变长离散消息 | 默认单写者/单读者 | Send/Receive 可阻塞 | **Stream Buffer + 消息长度前缀** | 变长报文/帧 |
| Queue Set | “哪个对象就绪” | 一个任务等待多个对象 | Select 可阻塞 | Queue/sem 成员把“成员句柄”送入集合 Queue | 多路事件复用 |

### 3.1 Queue、Semaphore、Mutex 为什么看起来不同，源码却是一家

`queue.c` 的 `Queue_t` 同时承载三类对象。

普通 Queue 使用：

`pcHead/pcWriteTo + QueuePointers_t + xTasksWaitingToSend + xTasksWaitingToReceive + uxMessagesWaiting`

Semaphore 不存数据，所以源码明确规定 `uxItemSize = 0`；Binary Semaphore 只是容量为 1，Counting Semaphore 的 `uxMessagesWaiting` 就自然变成计数。

Mutex 仍然复用 `Queue_t`，但 union 切换到：

`SemaphoreData_t = { xMutexHolder, uxRecursiveCallCount }`

同时 TCB 中的 `uxBasePriority/uxMutexesHeld` 配合实现**优先级继承**。

一句话：

`Queue 传数据；Semaphore 传“次数”；Mutex 传“资源所有权”。底座却都是 Queue_t。`

### 3.2 Task Notification：为什么最快

Queue/Semaphore 要有一个独立内核对象，还要维护对象自己的等待链表；Task Notification 不需要单独创建对象，因为：

```text
TCB
 ├─ ulNotifiedValue[]   // 32-bit payload/count/bitset
 └─ ucNotifyState[]     // waiting / received 状态
```

发送通知时直接修改目标任务 TCB；若目标任务正等待通知，就从阻塞链表移回 Ready List。

`eSetBits / eIncrement / eSetValueWithOverwrite / eSetValueWithoutOverwrite / eNoAction`
让同一机制可以模拟：事件位、计数信号量、邮箱值、纯唤醒。

代价也很明确：它天然绑定**指定接收任务**，不像 Queue 那样是独立共享邮箱。

### 3.3 Event Group：它解决的是“组合条件”，不是数据搬运

一个 Event Group 的核心只有：

```text
uxEventBits
xTasksWaitingForBits
```

任务可以等：

- 任意 bit 满足：`A || B || C`
- 所有 bit 满足：`A && B && C`

因此它适合“网络已连接 + 时间已同步 + 配置已加载”这种**状态组合**，而不是传结构体。

它与 Task Notification 的 bit 模式区别：Notification 的 bit 属于**某个任务自己的 TCB**；Event Group 是独立对象，可让多个任务围绕同一组状态位同步。

### 3.4 Stream Buffer 与 Message Buffer：一套环形缓冲，两种语义

`StreamBuffer_t` 的本体就是：

`head + tail + length + trigger level + buffer + waiting sender/receiver`

默认实现用 Task Notification 唤醒对端，因此要求 `configUSE_TASK_NOTIFICATIONS=1`。

二者区别：

```text
Stream Buffer:
[A][B][C][D][E]...          // 只认字节流，没有报文边界

Message Buffer:
[len][payload][len][payload] // 在同一 ring buffer 上保存离散消息边界
```

而且官方实现有一个非常重要的约束：**默认只支持单 writer + 单 reader**。如果多写者/多读者，应用必须额外串行化访问。

### 3.5 Queue Set：它不是“新的数据通道”，而是 select/poll

假设任务同时要等：

- UART Queue
- CAN Queue
- Binary Semaphore

不用轮询三个对象，可以把它们加入 Queue Set。任意成员变为可读/可获取时，集合告诉等待任务“哪个成员 ready”，然后任务再去对应对象真正取数据。

所以它的模型是：

`Queue Set = FreeRTOS 对多个 Queue/Semaphore 的事件多路复用器`

---

## 4. 最终心智模型

FreeRTOS 内核这三部分可以压缩成三个模型：

### 任务模型

`Task = Stack + TCB`

`TCB = 上下文入口 + 调度链表节点 + 优先级 + IPC状态 + 可选扩展状态`

### 内存模型

`Heap = 空闲空间的数据结构 + 分配策略 + 回收/合并策略 + 物理内存拓扑`

从 heap_1 到 heap_5，就是逐步增加“free、合并、多区域”的能力。

### IPC 模型

```text
要传固定记录      -> Queue
只要通知/计数      -> Task Notification 或 Semaphore
要保护共享资源     -> Mutex
要等待多个状态位   -> Event Group
要传连续字节       -> Stream Buffer
要传变长报文       -> Message Buffer
要同时等多个对象   -> Queue Set
```

工程上最值得记住的底层关系是：

```text
Queue
 ├─ Queue
 ├─ Binary/Counting Semaphore
 └─ Mutex/Recursive Mutex

TCB
 └─ Task Notification

StreamBuffer_t
 ├─ Stream Buffer
 └─ Message Buffer
      └─ 默认借 Task Notification 完成阻塞端唤醒

EventGroup_t
 └─ Event bits + waiting list
```

这张关系图比背 API 更重要：**FreeRTOS 看似有很多 IPC，真正的内核实现底座只有几套。**

## 5. 源码定位

- `tasks.c`：TCB、Task Notification、调度链表
  - https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d/tasks.c
- `queue.c`：Queue/Semaphore/Mutex/Queue Set 共用底座
  - https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d/queue.c
- `event_groups.c`
  - https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d/event_groups.c
- `stream_buffer.c`
  - https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d/stream_buffer.c
- `message_buffer.h`
  - https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d/include/message_buffer.h
- `portable/MemMang/heap_1.c ... heap_5.c`
  - https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d/portable/MemMang
