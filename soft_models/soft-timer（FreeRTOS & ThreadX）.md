## 1. Implement FreeRTOS
 - 核心组件： 插入缓存队列 + 有序List + daemon thread
 - 核心执行场景：用户定时，通过queue缓冲并唤醒daemon线程排序插入到list；更新全局最近唤醒时间点；然后tick中断中或者软中断中执行判断是否
	 最近定时器到达，到达则进行CB及全局最近定时点更新；


## 2. Implement ThreadX
 - 采用0-31哈希方式进行管理
 - 执行场景：用户定时，计算未来时间点并根据哈希计算插入到对应的槽位；然后每个tick的周期中进行tick的哈希计算，进行array[value]是否有timer的判断，
 - 如果有就进行处理。

---
### 本质就是添加一堆timer对象，到达之后执行CallBack

# From AI：

你的理解**总体方向对，但 FreeRTOS 执行路径有一个关键错误，ThreadX 的“哈希”叫法也需要修正**。

## 1. FreeRTOS Software Timer

- **核心组件**：Timer Command Queue + 按到期时间排序的 List + Timer Daemon Task。
- **执行流程**：
    1. 用户调用 `xTimerStart()/Reset()/ChangePeriod()` 等 API。
    2. 操作请求先写入 **Timer Command Queue**。
    3. 唤醒 Timer Daemon，由 Daemon 计算到期时间，并按到期时间插入有序 List。
    4. Daemon 获取 **最近到期 Timer**，然后阻塞等待：
        - 新的 Timer Command；或
        - 最近 Timer 到期。
    5. 到期后 Daemon 被唤醒，取出 Timer 并执行 Callback；周期 Timer 再重新插入 List。

**关键修正：**

```
Tick ISR 不检查 Timer List
Tick ISR 不执行 Timer Callback
```

Tick ISR 主要维护系统 tick 和任务超时；当 Daemon 的阻塞时间到达时，使其进入 Ready 状态。

可以记成：

```
APP
 ↓
Command Queue
 ↓
Timer Daemon
 ↓
Sorted Timer List
 ↓
阻塞到最近 Deadline
 ↓
Daemon Callback
```

核心思想：

> **把 Timer 按 Deadline 排序，只睡到最近的 Deadline。**

---

## 2. ThreadX Software Timer(倍率+圈数)

- **核心组件**：固定槽位的 **Timing Wheel（时间轮）**，默认 32 个槽 `0~31`。
- 不建议称为“哈希表”，虽然计算方式类似取模映射，本质上是 **环形时间轮**。

执行流程：

1. 用户启动 Timer。
2. 根据：
    
    ```
    当前槽 + timeout
    ```
    
    计算 Timer 应挂入哪个槽。
3. Timer 直接挂入对应槽的链表，通常为 **O(1)**。
4. 每发生一个 Tick：
    
    ```
    current_slot++
    ```
    
5. 检查当前槽：
    - 空 → 继续；
    - 有 Timer → 进入到期处理。
6. 默认由 **ThreadX Timer Thread** 执行 Callback；配置 `TX_TIMER_PROCESS_IN_ISR` 后可以直接在 ISR 中处理。

模型：

```
             Tick
              ↓
[0][1][2][3]...[31]
         ↑
    current slot
         │
   检查本槽 Timer
         ↓
   Timer Thread
         ↓
     Callback
```

对于超过 32 tick 的长定时器，并不是直接拥有更大的数组，而是经过一轮后减少 remaining ticks，再重新挂入时间轮。

---

## 最终对比记忆

|维度|FreeRTOS|ThreadX|
|---|---|---|
|数据结构|**有序 Deadline List**|**Timing Wheel**|
|启动 Timer|Command Queue → Daemon|直接计算槽并插入|
|插入|O(n)|通常 O(1)|
|Tick 是否检查 Timer 槽|否|**是**|
|到期判断主体|Timer Daemon|Tick + Timer Wheel|
|Callback 默认上下文|Daemon Task|Timer Thread|
|核心思想|**最近 Deadline 到了再醒**|**每 Tick 转一格检查槽**|

一句话记忆：

> **FreeRTOS = 排好闹钟，睡到最近一个闹钟响。**  
> **ThreadX = 一个 32 格时间轮，每个 Tick 转一格，看这一格有没有闹钟。**