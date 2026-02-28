## 1. API & macro

AUTOSAR OS 继承自 OSEK/VDX 规范，并扩展了多核、内存保护及时间保护功能。以下是 OS 规范中**必要的 API 服务及核心接口宏定义**，涵盖了任务控制、资源管理、事件处理、定时器与告警、调度表、保护机制及系统生命周期管理。

---

## 一、 核心 API 服务列表

| **分类**                   | **API 名称**              | **功能简述**                             | **关键参数/返回值**                       |
| ------------------------ | ----------------------- | ------------------------------------ | ---------------------------------- |
| **任务控制 (Task Control)**  | `ActivateTask`          | 将指定任务从 **Suspended** 状态转为 **Ready**。 | `TaskID`                           |
|                          | `TerminateTask`         | 结束当前正在运行的任务，并转为 **Suspended**。       | -                                  |
|                          | `ChainTask`             | 结束当前任务并立即激活另一个任务。                    | `TaskID`                           |
|                          | `Schedule`              | 显式触发调度器，检查是否有更高优先级任务。                | -                                  |
|                          | `GetTaskID`             | 获取当前正在运行任务的 ID。                      | `TaskRefType`                      |
|                          | `GetTaskState`          | 查询指定任务的当前状态（Running/Ready等）。         | `TaskID`, `StateRefType`           |
| **资源管理 (Resource)**      | `GetResource`           | 获取指定资源，利用 **PCP（优先级天花板）** 防止反转。      | `ResID`                            |
|                          | `ReleaseResource`       | 释放持有的资源，恢复任务原有优先级。                   | `ResID`                            |
| **事件处理 (Event)**         | `SetEvent`              | 向特定任务发送信号（仅适用于 Extended Task）。       | `TaskID`, `Mask`                   |
|                          | `ClearEvent`            | 清除当前任务的特定事件位。                        | `Mask`                             |
|                          | `GetEvent`              | 获取指定任务当前挂起的事件掩码。                     | `TaskID`, `EventRefType`           |
|                          | `WaitEvent`             | 挂起当前任务，直到收到指定事件位。                    | `Mask`                             |
| **告警管理 (Alarm)**         | `GetAlarmBase`          | 获取 Alarm 所属 Counter 的特征参数。           | `AlarmID`, `BaseRefType`           |
|                          | `GetAlarm`              | 获取指定告警距离触发还剩多少个 Tick。                | `AlarmID`, `TickRefType`           |
|                          | `SetRelAlarm`           | 设置相对告警，在偏移量 $x$ 之后触发。                | `AlarmID`, `increment`, `cycle`    |
|                          | `SetAbsAlarm`           | 设置绝对告警，在 Counter 达到特定值时触发。           | `AlarmID`, `start`, `cycle`        |
|                          | `CancelAlarm`           | 取消已设置的告警。                            | `AlarmID`                          |
| **调度表 (Schedule Table)** | `StartScheduleTableRel` | 基于相对时间启动预定义的调度表。                     | `SchedID`, `offset`                |
|                          | `NextScheduleTable`     | 设置当前调度表结束后自动切换的下一个表。                 | `SchedID`, `NextSchedID`           |
|                          | `StopScheduleTable`     | 停止正在运行的调度表。                          | `SchedID`                          |
| **保护与控制 (Protection)**   | `CheckObjectAccess`     | 检查 OS-Application 是否有权访问特定对象。        | `ApplID`, `ObjectType`, `ObjectID` |
|                          | `GetApplicationID`      | 获取当前正在执行的代码所属的应用 ID。                 | -                                  |
|                          | `CallTrustedFunction`   | 从非信任区域调用受信任的预定义函数。                   | `FunctionIndex`, `FunctionParams`  |
| **系统生命周期**               | `StartOS`               | 初始化并启动 OS 内核，进入第一个任务。                | `AppModeType`                      |
|                          | `ShutdownOS`            | 强制关闭 OS，通常进入无限循环或重启。                 | `StatusType`                       |

---

## 二、 核心接口宏与声明宏

在 AUTOSAR 规范中，为了保证移植性，**必须**使用规范定义的宏来声明任务、中断和常量。

| **宏定义名称**                   | **用途说明**                    | **示例用法**                      |
| --------------------------- | --------------------------- | ----------------------------- |
| **`TASK(name)`**            | **声明/定义任务**。确保编译器使用正确的函数原型。 | `TASK(MyTask) { ... }`        |
| **`ISR(name)`**             | **声明/定义 2 类中断**。由 OS 接管上下文。 | `ISR(Timer_ISR) { ... }`      |
| **`DeclareTask(name)`**     | 外部声明任务 ID，通常在 .h 中使用。       | `DeclareTask(Task_A);`        |
| **`DeclareResource(name)`** | 外部声明资源 ID。                  | `DeclareResource(Res_SPI);`   |
| **`DeclareEvent(name)`**    | 外部声明事件 ID。                  | `DeclareEvent(Ev_DataReady);` |
| **`DeclareAlarm(name)`**    | 外部声明告警 ID。                  | `DeclareAlarm(Alarm_10ms);`   |
| **`INVALID_TASK`**          | 常量宏，表示无效的任务句柄。              | `if (id == INVALID_TASK)`     |
| **`OSDEFAULTAPPMODE`**      | 默认应用模式，用于 `StartOS` 的参数。    | `StartOS(OSDEFAULTAPPMODE);`  |

---

## 三、 状态与错误码（核心返回值）

所有 API 均应严格遵循 `StatusType` 返回值规范：

- **`E_OK`**: 调用成功。
    
- **`E_OS_ACCESS`**: 访问权限错误（如试图激活不属于本应用的 Task）。
    
- **`E_OS_ID`**: 传入的 ID（Task/Resource 等）无效。
    
- **`E_OS_LIMIT`**: 达到激活限制（如 Task 已经处于 Ready 状态且不支持排队）。
    
- **`E_OS_RESOURCE`**: 资源占用冲突（如任务结束前未释放资源）。
    
- **`E_OS_STATE`**: 对象处于错误状态（如对未启动的告警执行取消操作）。
    

---

## 四、 专家核对说明

1. **静态性**：API 不包含 `CreateTask`。所有 Task 必须在 `.arxml` (或 OSEK 的 `.oil`) 中配置，由工具静态生成。
    
2. **中断分类**：规范严格区分 **Category 1**（绕过 OS，禁止调用 OS API）和 **Category 2**（OS 管理，可调用部分 API）。
    
3. **多核扩展**：对于多核系统，还需包含 `GetCoreID()` 和 `StartCore()` 等跨核控制接口。
    

## 2. structures （AUTOSAR OS 核心必要类型与结构列表）

一个关键的技术细节：**AUTOSAR OS 规范（SWS OS）本质上是一个接口规范，而非实现规范。**

这意味着规范定义了大量的**抽象数据类型（Data Types）**，但在 C 语言底层，这些类型通常被定义为 `uint32` 或结构体指针。为了保证“准确、无遗漏”，我将根据规范中定义的**标准类型（Standard Types）和配置对象（Configuration Objects）**，整理出 OS 运行所必须依赖的关键结构与类型定义。

以下类型是符合 AUTOSAR 规范的 OS 实现中必须存在的，它们构成了 API 的参数基础和内部管理逻辑。

|**分类**|**类型名称 (Type)**|**属性说明**|**功能简述**|
|---|---|---|---|
|**基础状态**|`StatusType`|`unsigned char`|所有 OS API 的返回值类型（如 `E_OK`, `E_OS_ID`）。|
|**任务管理**|`TaskType`|抽象句柄/ID|唯一标识一个任务（Task）。|
||`TaskRefType`|指针/引用|指向 `TaskType` 的指针，用于 `GetTaskID`。|
||`TaskStateType`|枚举/数值|表示任务状态：`RUNNING`, `WAITING`, `READY`, `SUSPENDED`。|
||`TaskStateRefType`|指针/引用|用于 `GetTaskState` API 获取状态返回。|
|**资源管理**|`ResourceType`|抽象句柄/ID|唯一标识一个资源（Resource）。|
|**事件管理**|`EventMaskType`|位掩码 (Bitmask)|用于 Extended Task 的事件触发与清除。|
||`EventMaskRefType`|指针/引用|用于 `GetEvent` 获取当前挂起的事件位。|
|**时间/告警**|`TickType`|计数单位|表示 Counter 的计数值，类型长度取决于硬件（通常 32位）。|
||`TickRefType`|指针/引用|用于获取当前 Tick 值的指针。|
||`AlarmBaseType`|**结构体 (Struct)**|包含：`maxallowedvalue`, `ticksperbase`, `mincycle`。|
||`AlarmBaseRefType`|指针/引用|指向 `AlarmBaseType` 的指针，用于 `GetAlarmBase`。|
||`AlarmType`|抽象句柄/ID|唯一标识一个告警（Alarm）。|
|**调度表**|`ScheduleTableType`|抽象句柄/ID|唯一标识一个调度表（Schedule Table）。|
||`ScheduleTableStatusType`|枚举/数值|状态包括：`STOPPED`, `RUNNING`, `NEXT`, `WAITING`。|
|**应用/保护**|`ApplicationType`|抽象句柄/ID|唯一标识一个 OS-Application（用于内存隔离）。|
||`ObjectAccessType`|枚举|表示是否有权访问：`ACCESS`, `NO_ACCESS`。|
||`ObjectTypeType`|枚举|标识对象种类：`OBJECT_TASK`, `OBJECT_ISR`, `OBJECT_ALARM` 等。|
||`RestartType`|枚举|定义 Application 出错后的行为：`RESTART`, `NO_RESTART`。|
|**系统控制**|`AppModeType`|抽象句柄/ID|用于 `StartOS` 的模式选择（如 `OSDEFAULTAPPMODE`）。|
||`CoreIdType`|整数/ID|**多核特有**，标识当前 CPU 核心的物理/逻辑 ID。|
||`SpinlockIdType`|抽象句柄/ID|**多核特有**，跨核同步使用的自旋锁标识。|

---

## 深度核对：关于“结构体”的实现细节说明

在实际的 Cortex-M7 开源实现（如 Trampoline）中，上述抽象类型通常对应以下具体的 C 结构体设计，这也是你“开发”时需要关注的：

### 1. 任务控制块 (Task Control Block, TCB)

虽然规范只要求 `TaskType`，但后台必须有一个结构体来维护：

- **Stack Pointer**: 当前任务的栈顶指针。
    
- **Static Priority**: 静态定义的优先级。
    
- **Running Priority**: 动态优先级（获取资源后提升）。
    
- **Context**: 寄存器现场保护区。
    

### 2. 告警基础结构 (AlarmBaseType)

这是规范中明确要求的结构体类型，定义如下：

C

```
typedef struct {
    TickType maxallowedvalue; // Counter能达到的最大值
    TickType ticksperbase;    // 达到一个Tick所需的硬件周期数
    TickType mincycle;        // 周期性告警允许的最小周期
} AlarmBaseType;
```

### 3. 中断向量描述 (ISR Config)

对于 Category 2 中断，OS 内部通常维护一个结构体，包含中断优先级、处理函数指针以及它所属的 OS-Application。

---

## 总结与准确性核对

1. **静态性验证**：所有 ID 类型（TaskType, AlarmType 等）在 AUTOSAR 中本质上是索引或指针，确保了系统无动态分配。
    
2. **遗漏检查**：已包含多核（Spinlock/CoreID）和安全保护（ApplicationType）相关必要类型。
    
3. **一致性**：所有命名严格遵循 `AUTOSAR_SWS_OS.pdf` 中的定义。
    

**你现在已经掌握了 API 和 数据结构。接下来，是否需要我为你展示如何在配置文件（OIL/ARXML）中定义这些结构，并自动生成对应的 C 语言初始化代码？**