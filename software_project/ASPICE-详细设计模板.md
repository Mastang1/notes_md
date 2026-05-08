# 软件详细设计说明书 (Software Detailed Design Specification)

## 1. 软件详细设计描述概述 (Overview of Software Detailed Design)

### 1.1 编制目的与适用范围 (Purpose and Applicability)

> **[审查员视角]**：确认本文档在 ASPICE V 模型右侧的承接合法性，防止设计越权。
> 
> **[编制指导]**：明确本说明书归属的底层驱动模块，强制声明上游架构输入基线。

**[IPCF 演练 Demo]**:

本说明书旨在确立 NXP 异构多核通信框架（IPCF, Inter-Platform Communication Framework）驱动模块的软件详细设计规范。

本文件作为实现阶段（SWE.3 Implement）的核心基线，严格约束于《IPCF_Software_Architecture_Design_v2.0》（SWE.2 架构说明书）所下发的组件边界、端口拓扑与接口契约。本文档将其转化为具体的 C 语言编译单元结构、微观数据控制流与底层物理防错机制，并作为后续 SWE.4 软件单元测试（Unit Testing）用例生成的唯一参照标准。

### 1.2 参考标准与合规性基线 (Reference Standards and Compliance Baselines)

**[IPCF 演练 Demo]**:

- 过程合规基线：Automotive SPICE PAM 4.0 (SWE.3 Base Practices 1-4)
    
- 安全防错基线：ISO 26262-6:2018 (Part 6: Product development at the software level)
    
- 代码实现基线：MISRA C:2012 / AUTOSAR C++14 编码指导方针
    

---

## 2. 架构追溯与物理单元拓扑 (Architecture Traceability and Physical Unit Topology)

_[`ASPICE 4.0 SWE.3 BP4`: Ensure consistency and establish bidirectional traceability]_

> **[审查员视角]**：这是斩断系统“幽灵代码（Orphan Code）”与“镀金设计（Gold Plating）”的防弹矩阵。
> 
> **[编制指导]**：建立一张严密的追溯矩阵，将 SWE.2 逻辑组件（Component）降维映射至 SWE.3 的编译级物理单元（Compilation Unit / Source Files）。

### 2.1 组件到单元的降维映射矩阵 (Component-to-Unit Resolution Mapping Matrix)

**[IPCF 演练 Demo]**:

本层设计将架构组件实体化为独立的高内聚 C 语言物理源文件。

|**架构逻辑组件 ID (SWE.2)**|**编译物理单元 ID (SWE.3)**|**物理源文件锚定 (Source Files)**|**微观物理职责边界 (Micro-Physical Responsibilities)**|
|---|---|---|---|
|`COMP_IPC_QUEUE_MGR`|`UNIT_IPCF_RINGBUF`|`ipcf_queue.c`<br><br>  <br><br>`ipcf_queue.h`|承接无锁环形队列的内存实体管理，执行指针偏移计算与 CPU 缓存隔离。|
|`COMP_IPC_HAL`|`UNIT_IPCF_HW_MSCM`|`ipcf_hw_mscm.c`<br><br>  <br><br>`ipcf_hw_mscm.h`|承接特定 SOC 外设控制，管理跨核 IRQ 路由及中断服务例程（ISR）的硬件上下文保护。|
|`COMP_IPC_API`|`UNIT_IPCF_CTRL`|`ipcf_api.c`<br><br>  <br><br>`ipcf_api.h`|承接通道实例生命周期控制、架构暴露接口的参数断言与异常派发。|

---

## 3. 软件单元静态结构视图 (Software Unit Static Structure View)

_[`ASPICE 4.0 SWE.3 BP1`: Develop software detailed design (Static)]_

> **[审查员视角]**：检查开发人员是否针对跨核、DMA、Cache 等物理硬件特性进行了底层数据构造防御。
> 
> **[编制指导]**：绝对禁止列举普通的局部变量。必须详细定义全局跨单元控制块（Control Block）、硬件对齐约束（Alignment）、防编译器乱序机制（Volatile）及特殊的内存段链接策略（Linker Pragma）。

### 3.1 跨核及底层关键数据结构定义 (Cross-Core and Low-Level Critical Data Structures)

**[IPCF 演练 Demo]**:

针对 `UNIT_IPCF_RINGBUF`，为满足零拷贝架构下 A/M 核的物理一致性，其控制头实体结构设计如下：

C

```
/** 
 * @brief IPC 共享内存通道控制块实体
 * @implements UNIT_IPCF_RINGBUF_STRUCT_01
 * @note 物理链接约束：必须由 Linker 分配至 .mcal_shared_ram_nc (Non-Cacheable) 物理内存段，严禁开启 Data Cache。
 */
typedef struct __attribute__((aligned(64))) { /* 硬件防错：强制 64 字节对齐，匹配 A53 Cache Line，根除 False Sharing 踩踏风险 */
    uint32_t magicNum;             /* [0x00] 初始化握手校验字 */
    uint32_t ringDepth;            /* [0x04] 队列深度界限 (算力约束：强制限制为 2^N) */
    
    /* 编译防优防御：跨核指针必须加 volatile，防止 GCC 优化为通用寄存器缓存导致状态机挂死 */
    volatile uint32_t headIndex;   /* [0x08] 写入游标控制字 */
    volatile uint32_t tailIndex;   /* [0x0C] 读取游标控制字 */
    
    uint32_t dataOffsetArray[];    /* [0x10] 柔性负载描述符数组 (仅存偏移量，兑现零拷贝契约) */
} Ipcf_ShmChannelCb_Type;
```

---

## 4. 软件单元物理接口与内部实现规约 (Software Unit Physical Interface and Implementation Specification)

_[`ASPICE 4.0 SWE.3 BP2`: Define interfaces of software units]_

> **[审查员视角]**：绝不重写 API 签名，专注于规约 API 被调用时的内存断言、中断锁定、硬件执行边界。
> 
> **[编制指导]**：针对继承自架构的外部 API，说明其内部物理执行约束。针对单元私有的静态 API，说明其辅助算法规约。

### 4.1 外部提供接口的内部实现规约 (Internal Implementation Specs of Provided Interfaces)

**[IPCF 演练 Demo]**:

本节规定承接自 `P_QueueAccess` 架构端口的具体 API 物理实现约束。其签名契约见 SWE.2，此处规定其实现法则。

#### 4.1.1 接口实现实体：`Ipcf_Queue_Push`

- **承接架构接口契约**: `I_QueueOp::Push`
    
- **输入参数物理断言 (Input Parameter Physical Assertions)**:
    
    - `chId`: 内部实现必须进入函数即调用 `IPCF_DEV_ASSERT(chId < IPCF_MAX_SUPPORTED_CHANNELS)`，拦截野指针通道。
        
    - `dataOffset`: 内部必须执行按位与校验 `(dataOffset & 0x03) == 0`，若未达到 32-bit 对齐，直接触发 CPU Exception。
        
- **微观执行上下文与临界区约束 (Micro-Execution Context & Critical Section)**:
    
    - **重入性与抢占防御**: 本函数在更新 `headIndex` 时为临界区操作。由于是同核多任务并发调用，实现时**必须且只能**调用 `Os_SuspendAllInterrupts()` 禁用全局中断，禁止使用 Mutex 避免优先级反转（Priority Inversion）。
        
- **后置物理动作 (Post-Physical Actions)**:
    
    - 在写操作落盘前，必须插入 ARM 汇编级内建函数 `__DMB()`，否则视为严重软件失效。
        

### 4.2 本地静态辅助方法规约 (Local Static Auxiliary Methods Specification)

**[IPCF 演练 Demo]**:

本单元不对外暴露的、用于支撑复杂内部逻辑的静态私有方法。

- **方法原型**: `static inline uint32_t Ipcf_CalcNextWrapIndex(uint32_t currIdx, uint32_t maxDepth);`
    
- **算法约束**: 鉴于该函数部署于 ISR 热点路径（Hot Path），实现时禁止调用编译器除法指令（`sdiv/udiv`），必须使用按位掩码 `(currIdx + 1) & (maxDepth - 1)` 实现快速回绕。
    

---

## 5. 软件单元动态行为与微观时序 (Software Unit Dynamic Behavior and Micro-Timing)

_[`ASPICE 4.0 SWE.3 BP1`: Develop software detailed design (Dynamic)]_

> **[审查员视角]**：拒绝代码级的流水账。针对系统级状态流转、跨单元微观调度解耦、以及包含底层寄存器屏障操作的高风险区域提供图形化建模。
> 
> **[编制指导]**：强制规定必须具备：状态迁移图（生命周期）、时序图（并发交互解耦）、活动图（防御性底层算法）。

### 5.1 状态迁移逻辑图 (State Transition Logic Diagram)

**[IPCF 演练 Demo]**:

定义通道控制块在双核不对称启动时的握手生命周期与异常降级策略。

代码段

```
stateDiagram-v2
    [*] --> STATE_UNINIT
    STATE_UNINIT --> STATE_HANDSHAKE_TX : Local Init Execution (Write Magic)
    STATE_HANDSHAKE_TX --> STATE_READY : Polling: Remote Magic Confirmed
    STATE_READY --> STATE_READY : Normal Tx/Rx Exchange
    STATE_READY --> STATE_FAULT : Trap: Bus Fault / Invalid Index Detected
    STATE_FAULT --> STATE_UNINIT : Recovery Sequence Triggered
```

### 5.2 微观并发交互时序图 (Micro-Concurrency Interaction Sequence Diagram)

**[IPCF 演练 Demo]**:

描述底半部（Bottom-Half）任务调度设计，兑现架构中“ISR 中断不携带沉重负载”的实时性要求。

代码段

```
sequenceDiagram
    participant HW as MSCM Peripheral Registers
    participant ISR as UNIT_IPCF_HW_MSCM (ISR Top-Half)
    participant RTOS as OS Scheduler
    participant Queue as UNIT_IPCF_RINGBUF (Task Bottom-Half)

    HW->>ISR: Core IRQ Asserted
    activate ISR
    ISR->>HW: Write 1 to clear Interrupt Pending
    ISR->>RTOS: OS_SetEvent(RX_PAYLOAD_EVENT)
    Note over ISR: WCET < 2us. Absolute no memory fetching.
    ISR-->>HW: IRET (Context Restore)
    deactivate ISR

    Note over RTOS,Queue: System ticks.. RTOS Preempts to IPC Task
    RTOS->>Queue: Wakeup IPC_RxTask()
    activate Queue
    Queue->>Queue: Safely fetch from Non-Cacheable RAM
    Queue-->>RTOS: Dispatch to App Layer
    deactivate Queue
```

### 5.3 核心底层防错算法序列 (Core Fault-Tolerant Algorithm Sequence)

**[IPCF 演练 Demo]**:

针对 `Ipcf_Queue_Push`。通过流程控制图强制定死指令乱序防御（Out-of-Order Execution Defense）的插入点。

代码段

```
graph TD
    A([Enter: Ipcf_Queue_Push]) --> B{Is `head + 1 == tail`?}
    B -- True --> C([Return IPCF_E_QUEUE_FULL])
    B -- False --> D[Store `dataOffset` to physical RAM]
    
    D --> E[<b style="color:red">HW Defense: Invoke __DMB()</b>]
    style E stroke:#f00,stroke-width:3px
    Note right of E: Force Write Buffer drain<br/>Guarantee payload visibility to Remote Core
    
    E --> F[Update `headIndex` cursor]
    
    F --> G[<b style="color:red">HW Defense: Invoke __DSB()</b>]
    style G stroke:#f00,stroke-width:3px
    Note right of G: Block instruction pipeline<br/>Ensure pointer is settled before IRQ assertion
    
    G --> H[Trigger MSCM Inter-Core Interrupt]
    H --> I([Return E_OK])
```

---

## 6. 详细设计资源评估与安全防护分析 (Detailed Design Resource Evaluation and Safety Mechanism Analysis)

_[`ASPICE 4.0 SWE.3 BP3`: Analyze software detailed design]_

> **[审查员视角]**：证明详细设计不仅实现了功能，而且没有击穿硬件天花板，且具备应对硬件瞬态故障（Transient Hardware Faults）的能力。
> 
> **[编制指导]**：输出硬核的性能预估数据（ROM/RAM/WCET）及符合功能安全（ISO 26262 ASIL X）的干扰免除机制。

### 6.1 物理资源消耗边界分析 (Physical Resource Consumption Boundary Analysis)

**[IPCF 演练 Demo]**:

- **调用栈压力边界 (Stack Consumption Limit)**: 核心收发路径杜绝递归调用，局部变量类型严控。`UNIT_IPCF_HW_MSCM` 的 ISR 完整上下文切换及执行深度测试预估 $< 64\text{ Bytes}$，免疫栈溢出风险。
    
- **最坏执行时间 (Worst-Case Execution Time, WCET)**: `UNIT_IPCF_RINGBUF` 的无锁算法去除了系统调用（System Call）开销，分支预测单一。400MHz 频率下，单次入队操作（包含内存屏障）周期消耗预估 $< 2.5\mu s$，保障应用层调用绝对无感。
    

### 6.2 硬件失效免疫与容错机制 (Hardware Failure Immunity and Fault Tolerance Mechanisms)

**[IPCF 演练 Demo]**:

- **跨核指针越界免疫 (ISO 26262 Freedom from Interference)**:
    
    - **风险模型**: 远端核因系统奔溃或恶意篡改，写入了一个巨大的 `tailIndex` 传递给本核。
        
    - **防御设计**: 在读取 `tailIndex` 进行任何数组基址偏移计算前，详细设计强制要求实行 `safe_index = raw_index & (ringDepth - 1)` 掩码截断。
        
    - **安全结论**: 无论外部输入何种异常值，内部物理寻址永远被死死钳制在预分配的控制块 RAM 空间内，实现了物理级的内存越界（Memory Corruption）隔离防护。