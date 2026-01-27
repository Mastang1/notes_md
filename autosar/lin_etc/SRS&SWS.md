# 1. 分清两者的使用对象
## ==0 基于Lin_sws及fw api进行driver开发==


简单来说：**SRS 是“甲方需求”，SWS 是“实施图纸”。**

以下是详细对比，以及针对你（MCAL 开发程序员）的实操指南。

---

### 1. 两个文档的详细对比

|**特性**|**AUTOSAR_SRS_LIN.pdf**|**AUTOSAR_SWS_LINDriver.pdf**|
|---|---|---|
|**全称**|**S**oftware **R**equirement **S**pecification|**S**oftware **W**eird **S**pecification (实际上是 **S**oftware **S**pecification)|
|**中文名**|软件需求规格说明书|软件规格说明书（详细设计）|
|**核心问题**|**做什么 (What)**|**怎么做 (How)**|
|**内容示例**|“LIN 驱动必须支持波特率配置。”<br><br>  <br><br>“LIN 驱动必须能发送帧。”|“函数名必须叫 `Lin_SendFrame`。”<br><br>  <br><br>“波特率配置参数名叫 `LinChannelBaudRate`，类型是 `uint16`。”|
|**主要受众**|系统架构师、需求工程师、主机厂（OEM）|**MCAL 开发程序员（你）**、软件测试人员、集成工程师|
|**你的关注度**|⭐⭐ (作为背景知识了解)|⭐⭐⭐⭐⭐ (写代码时的圣经)|

---

### 2. 文档具体是干什么的？

#### **AUTOSAR_SRS_LIN.pdf (需求)**

- **用途：** 定义了 LIN 驱动模块在高层面上必须满足的功能性约束。它通常比较抽象，不涉及代码细节。
    
- **谁用：**
    
    - **架构师：** 确认整个 BSW 层的设计是否满足了整车厂的需求。
        
    - **MCAL 开发者（你）：** 当你觉得 SWS 里的某个规定很离谱，或者 SWS 写得不清楚时，回过头查 SRS，看看原始需求到底是为了解决什么问题（Traceability）。
        

#### **AUTOSAR_SWS_LINDriver.pdf (规格/设计)**

- **用途：** 它是 SRS 的具体实现指南。它规定了 C 语言层面的所有细节：
    
    - **API 定义：** 函数名、参数、返回值（如 `Lin_GetStatus`）。
        
    - **配置参数：** EcuC (ECU Configuration) 的容器结构。
        
    - **行为逻辑：** 状态机跳转图、时序图。
        
    - **错误处理：** 开发错误（DET）和运行时错误（DEM）的 ID。
        
- **谁用：**
    
    - **MCAL 开发者（你）：** **这是你的核心工作文档。** 你写的每一行代码、定义的每一个头文件宏，都必须严格遵守这个文档。
        
    - **一致性测试工具：** 验证你的驱动是否符合 AUTOSAR 标准。
        

---

### 3. 作为 MCAL 程序员，如何确定协处理器 LIN 的开发需求？

假设你现在的任务是：**“基于某款 SoC 的 LIN 协处理器（或 LIN IP 核），开发符合 AUTOSAR 标准的 MCAL LIN 驱动”**。

你应该按照以下 **“三步走”** 策略来确定开发需求：

#### **第一步：以 SWS 为骨架 (Map Standard to Code)**

打开 `AUTOSAR_SWS_LINDriver.pdf`，这是你的“填空题”模板。你需要提取出所有必须实现的接口：

- **功能需求：** 必须实现 `Lin_Init`, `Lin_SendFrame`, `Lin_GoToSleep`, `Lin_Wakeup` 等标准 API。
    
- **数据结构：** 必须定义 `Lin_ConfigType` 结构体，且内容要符合 SWS 第 10 章（Configuration specification）的要求。
    
- **状态机：** 你的代码内部状态（UNINIT -> INIT -> SLEEP -> OPERATIONAL）必须完全复刻 SWS 中的状态图。
    

#### **第二步：以芯片手册为血肉 (Map Hardware to SWS)**

打开你的 **SoC 芯片参考手册 (Reference Manual / Register Map)**。

因为你是为“协处理器/IP 核”写驱动，你需要将 SWS 的标准接口映射到硬件寄存器操作上：

- **SWS 要求：** `Lin_SendFrame(Channel, PduInfo)`
    
- **硬件实现：** 查手册，找到 `LIN_DATA_REG` 和 `LIN_CTRL_REG`。
    
    - _思考：_ 如何把 `PduInfo` 里的数据塞进硬件的 FIFO？如何触发发送位？
        
- **SWS 要求：** `Lin_GetStatus(Channel)`
    
- **硬件实现：** 查手册，找到 `LIN_STATUS_REG`。
    
    - _思考：_ 硬件的 "Tx Complete" 标志位如何对应 AUTOSAR 的 `LIN_TX_OK` 状态？
        

#### **第三步：处理“Gap” (Gap Analysis)**

这是最难的地方。AUTOSAR SWS 是通用的，但你的协处理器硬件可能有特殊性。

- **场景 A（硬件太强）：** 你的 LIN 协处理器支持硬件自动重发，但 AUTOSAR SWS 可能没强制要求。
    
    - _决策：_ 作为一个符合标准的驱动，你可以默认关闭硬件的高级功能，或者通过私有的配置项（Vendor Specific）来开启。
        
- **场景 B（硬件太弱）：** SWS 要求支持 `Lin_Wakeup`（唤醒检测），但你的硬件没有唤醒中断。
    
    - _决策：_ 你需要用轮询（Polling）或者借助外部 GPIO 中断来模拟这个功能，以满足 SWS 的接口要求。
        

### 总结操作流

1. **左手拿 `SWS_LINDriver`**：列出所有必须写的 C 函数清单。
    
2. **右手拿 `SoC Reference Manual`**：查找每个函数底层需要读写哪些寄存器。
    
3. **中间放 `SRS_LIN`**：只有当 SWS 里某句话你读不懂（比如“为什么要在这个时候报错？”），或者你在做需求追踪表（Compliance Matrix）时，才去查 SRS。
    

**一句话建议：**

==先通读一遍 **SWS** 的 **Chapter 7 (Functional Specification)** 和 **Chapter 8 (API Specification)**，这就是你 90% 的开发需求来源。==

# 2. 