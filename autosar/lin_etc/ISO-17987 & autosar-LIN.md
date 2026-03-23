# `协议标准` 和 `软件实现标准` 的关系

## 1. 对比理解
### 第一部分：物理层与硬件基础 (Layer 0)

**规范对应：ISO 17987-4**
- todo: CAN & ISO-11898
这是一切通信的物理载体，主要涉及 SoC 片内外设与外部收发器。

- **硬件构成：**
    
    - **SoC 内部：** UART/LIN 控制器（通常带有专用的 LIN 硬件加速机制，如自动波特率检测、自动校验和生成）。
        
    - **外部收发器 (Transceiver)：** 负责将 SoC 输出的 TTL 逻辑电平（Tx/Rx）转换为 LIN 典型的单线 12V/24V 电池电平。主节点包含 1 kΩ 上拉电阻，从节点为 30 kΩ。
        
- **信号特性：** 显性（Dominant, 0，低电平），隐性（Recessive, 1，高电平）。最高波特率 20 kbps。
    
- **AUTOSAR 映射：**
    
    - 这一层主要是纯硬件。如果外部收发器是带有控制引脚（如 EN, WAKE）或通过 SPI 配置的智能芯片，则在 AUTOSAR 中由 **`LinTrcv` (LIN Transceiver Driver)** 模块控制其工作模式（Normal, Standby, Sleep）。
        

### 第二部分：微控制器抽象层 (MCAL) - 驱动核心 (Layer 1)

**规范对应：ISO 17987-3 (部分，仅涉及帧收发物理动作)**

这是底层驱动开发的“主战场”。`Lin` Driver 的核心原则是：**只做动作，不管时序，不问业务**。

- **核心职责：**
    
    - **寄存器级操作：** 直接操控 SoC 的 LIN/UART 控制器，触发 Break（同步间隔）、Sync Byte（同步字节）、PID（受保护标识符）的发送。
        
    - **数据流转：** 根据上层传入的 PDU（协议数据单元）指针，将数据填入发送 FIFO，或从接收 FIFO 提取数据。
        
    - **硬件校验与错误检测：** 启用硬件的 Classic/Enhanced Checksum 计算机制，处理位错误、校验错误或帧错误中断。
        
- **边界界定：**
    
    - `Lin` 驱动**不知道**当前发送的是无条件帧还是诊断帧。
        
    - `Lin` 驱动**没有**调度表（Schedule Table）的概念。
        
    - 它只提供原子级的 API，如 `Lin_SendFrame()`（触发一次收发过程）、`Lin_GetStatus()`（查询当前硬件状态：发送中/接收完成/睡眠中）、`Lin_GoToSleep()`（发送休眠命令）。
        
- **中断处理：** 处理收发完成中断、唤醒中断（识别总线上的低电平唤醒脉冲），并将状态上报给接口层。
    

### 第三部分：ECU 抽象层 - 时序与调度控制器 (Layer 2)

**规范对应：ISO 17987-3 (核心 MAC 层机制)**

这一层由 **`LinIf` (LIN Interface)** 模块主导，它是 LIN 通信的大脑，负责将静态的配置文件转化为动态的总线时序。

- **调度表执行 (Schedule Table Manager)：**
    
    - 主节点的 `LinIf` 内部维护着状态机。在一个周期性的 Task 中，它严格按照时隙（Time Slot）轮询调度表，适时调用底层的 `Lin_SendFrame()`。
        
    - 在工具链（如 EB Tresos）中导入 LDF (LIN Description File) 后，所有的帧定义、调度表顺序、时隙时长都会被解析，并生成为 `LinIf` 的配置结构体代码（`LinIf_PBcfg.c` 等），而不会污染底层的 `Lin` 驱动。
        
- **主从任务分离：**
    
    - **主任务 (Master Task)：** 负责发送 Header (Break + Sync + PID)。
        
    - **从任务 (Slave Task)：** 负责在匹配到 PID 后，发送或接收 Response (Data + Checksum)。`LinIf` 需要根据配置决定是由本地节点填充数据，还是等待总线上的其他从节点应答。
        
- **状态与错误汇总：** 收集底层 `Lin` 上报的状态，处理事件触发帧的冲突逻辑。
    

### 第四部分：服务层 - 网络管理与诊断路由 (Layer 3)

**规范对应：ISO 17987-2 (TP 层), ISO 17987-3 (网络状态管理)**

这一层脱离了具体的帧收发，侧重于系统级的状态和长报文处理。

- **网络状态管理 (`LinSM` - LIN State Manager)：**
    
    - 负责全局的 Sleep/Wakeup 序列管理。
        
    - 当底层收到唤醒中断并层层上报后，`LinSM` 会接管唤醒流程，与通信管理器 (`ComM`) 和基础软件模式管理器 (`BswM`) 交互，决定是否将整个 ECU 切换至全运行状态，并指示 `LinIf` 启动调度表。
        
- **传输协议 (`LinTp` - LIN Transport Protocol)：**
    
    - 专为 UDS 诊断服务设计。由于 LIN 单帧 Payload 最大 8 字节，当遇到长诊断报文时，`LinTp` 负责将其分段（Segmentation）并通过首帧、连续帧机制发送，或者将接收到的分段重组（Reassembly）成完整的长报文。
        
    - 处理完毕后，将完整的 PDU 交给通用路由模块 `PduR`。
        

### 第五部分：应用层与信号解包 (Layer 4)

**规范对应：ISO 17987-5 (API), 应用级业务逻辑**

这是数据到达用户逻辑的最后一步。

- **通信服务 (`Com` - Communication)：**
    
    - `LinIf` 通过 `PduR` 将完整的 I-PDU 传递给 `Com` 模块。
        
    - `Com` 的核心作用是“拆包/打包”。它根据数据库（如 LDF/DBC 定义的大端/小端格式、起始位、长度），将字节流解析为具体的物理信号（Signal，比如“车门状态 = 1”）。
        
- **运行时环境 (`RTE`) 与应用层 (Application SWC)：**
    
    - `Com` 将解析好的信号通过 `RTE` 接口提供给应用层软件组件（SWC）。
        
    - 此时，应用层开发者只需要关心业务逻辑，完全感知不到底层是经过 LIN、CAN 还是以太网传输的。
        

---

**总结起来，数据的流动路径是：**

应用层变量 $\leftrightarrow$ `RTE` $\leftrightarrow$ `Com` (信号打包成PDU) $\leftrightarrow$ `PduR` $\leftrightarrow$ `LinIf` (根据调度表排队出栈) $\leftrightarrow$ `Lin` Driver (操控硬件发送 Header，处理收发动作) $\leftrightarrow$ SoC UART外设 $\leftrightarrow$ LIN 收发器 $\leftrightarrow$ 物理总线。

这套体系将复杂的时序和报文解析全部上移，保证了底层驱动的极度精简和可复用性。

在进行 MCAL 驱动开发或集成时，你是否遇到过 `LinIf` 的休眠/唤醒逻辑与底层硬件（如唤醒中断源的清除与使能）配合不畅，导致状态机卡死的问题？我们可以针对具体的硬件交互流程做进一步探讨。

## 2. todo
