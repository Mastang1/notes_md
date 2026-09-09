# MCU 综合基础知识库：Cortex-M 问题驱动版

## 0. 文档定位与适用边界

本文从来源文档 `MCU-base.pdf` 的 21 个问题出发，补充 MCU 岗位面试和工程开发中高频、能够反映底层能力的问题。主线是 Armv7-M 架构的 Cortex-M3/M4/M7；涉及芯片厂商实现的内容，以 **STM32F407** 为寄存器级实例。

> **最重要的边界**：Cortex-M 是处理器内核；RCC、EXTI、GPIO、DMA、片上 Flash、BOOT 引脚等属于具体 MCU 厂商的 SoC 实现。不能拿 STM32F407 的数值当成所有 Cortex-M 的规则。

本文中的“手册依据”均指 Arm、CMSIS、ST、NXP 或 Bosch 的官方资料。正文尽量直接保留官方寄存器名、位名和架构术语，但不大段复制手册原文；具体解释是对原文的忠实转述。

### 0.1 来源文档的四个主题

| 原主题 | 原问题数 | 本文位置 |
|---|---:|---|
| 中断系统 | 5 | 第 2 章 |
| 时钟系统 | 5 | 第 3 章 |
| 存储与启动 | 5 | 第 4 章 |
| Bootloader 与 IAP | 6 | 第 5 章 |

### 0.2 先纠正来源文档中的四个易错点

1. `volatile` 只约束编译器对该对象访问的优化，不提供原子性、临界区、跨核同步或 Cache 一致性。
2. “NVIC 默认分组”必须区分架构复位值和库初始化后的值。Armv7-M 的 `AIRCR.PRIGROUP` 复位值为 0；具体工程可能在启动代码/HAL 中改写。
3. 从向量表取出的复位处理函数地址必须满足 Thumb 状态要求，即入口值 bit[0] 为 1；“栈顶地址是偶数”不是充分的 App 合法性检查。
4. `.data/.bss` 初始化与 `SystemInit()` 的先后次序取决于工具链启动实现。CMSIS 只规定总体职责，不能把某一种启动文件顺序说成架构硬规则。

### 0.3 目录与推荐阅读顺序

- [1. Cortex-M 处理器与执行模型](#1-cortex-m-处理器与执行模型)
- [2. 中断与异常系统](#2-中断与异常系统)
- [3. 时钟、复位与低功耗](#3-时钟复位与低功耗)
- [4. 存储、链接与启动流程](#4-存储链接与启动流程)
- [5. Bootloader、IAP 与可靠升级](#5-bootloaderiap-与可靠升级)
- [6. 并发、原子操作、DMA 与 Cache](#6-并发原子操作dma-与-cache)
- [7. Fault、调试与系统可靠性](#7-fault调试与系统可靠性)
- [8. 高频外设问题](#8-高频外设问题)
- [9. Cortex-M 系列差异](#9-cortex-m-系列差异只记会改变工程结论的部分)
- [10. MCU 心智模型](#10-面试速记一个-mcu-的完整心智模型)
- [11. 官方手册索引](#11-官方手册索引)
- [附录 A：来源 PDF 问题覆盖检查](#附录-a来源-pdf-的-21-个问题覆盖检查)

若用于面试复习，先读第 2～5 章（完整覆盖来源 PDF），再读第 6～8 章新增问题；第 1 章负责补齐回答这些问题所需的内核前置知识。

---

# 1. Cortex-M 处理器与执行模型

## Q1：MCU、Cortex-M 内核和外设是什么关系？

**回答**：Cortex-M 是处理器 IP，定义寄存器、指令集、异常模型、NVIC、SysTick、SCB，以及可选 MPU/FPU/Cache 等。MCU 厂商把内核、Flash、SRAM、总线矩阵、时钟复位、电源管理和外设集成为一颗芯片。因此 `SCB->VTOR` 属于 Cortex-M 系统控制空间，而 `RCC->CR`、`GPIOA->MODER`、`FLASH->CR` 属于 STM32F407。

**工程判断**：遇到问题先问“它属于 core 还是 device”。core 问题查 Arm Architecture Reference Manual/Generic User Guide；device 问题查芯片 Reference Manual 和 Datasheet。

**记忆**：**内核负责执行和异常，芯片负责存储、时钟、电源和外设。**

**手册依据**：Cortex-M4 Generic User Guide，第 2 章；CMSIS-Core Overview；RM0090，第 1～3 章。[^cm4dug][^cmsis][^rm0090]

## Q2：Cortex-M4 有哪些核心寄存器？调用函数时各做什么？*

**回答**：`R0-R12` 为通用寄存器；`R13` 是 SP，物理上对应 MSP/PSP；`R14` 是 LR；`R15` 是 PC。`xPSR` 由 APSR、IPSR、EPSR 的视图组成。异常屏蔽和执行控制使用 `PRIMASK`、`BASEPRI`、`FAULTMASK`、`CONTROL`。按 AAPCS32，`r0-r3` 用于参数和结果且由调用者保存；`r4-r8、r10、r11、SP` 由被调用者保持，`r9` 的角色由平台约定，`r12` 是临时寄存器，`LR` 保存返回地址。

**寄存器检查**：调试器中查看 `PC/LR/xPSR/MSP/PSP/CONTROL`，比只看 C 调用栈更能判断异常前状态。

**记忆**：**R0～R3 传参，R4～R11 多数保现场，R13 栈、R14 返回、R15 执行。**

**手册依据**：PM0214 §2.1；AAPCS32 §6.1。[^pm0214][^aapcs]

## Q3：Thread mode、Handler mode、特权级是什么关系？*
>总结：俩模式：一个中断/异常；一个就是normal；然后这两种运行模式，可以设置特权级，主要是线程模式可以特权级；

**回答**：复位后处理器处于特权 Thread mode。所有异常处理程序运行在特权 Handler mode；普通程序运行在 Thread mode，可由 `CONTROL.nPRIV` 选择特权或非特权。非特权 Thread mode 不能直接清除 `nPRIV` 恢复特权，通常要通过 `SVC` 进入特权异常处理。`CONTROL.SPSEL` 在 **Thread mode 选择 MSP 或 PSP**；Handler mode 始终使用 MSP。

**工程用途**：RTOS 常让内核运行特权态、用户任务运行非特权态，并用 MPU 限制任务可访问区域。

**记忆**：**线程可降权，异常必特权；降权后靠 SVC 回内核。**

**手册依据**：PM0214 §2.2、§2.3；CMSIS Core Register Access。[^pm0214][^cmsis-reg]

## Q4：MSP 和 PSP 为什么要设计成两个栈？no

**回答**：MSP 是复位后的默认栈，也是 Handler mode 使用的栈；PSP 仅供 Thread mode 使用。两个栈使异常/内核栈与线程/任务栈隔离。异常入栈时使用被打断上下文当时选中的栈；进入 Handler 后执行代码使用 MSP。异常返回时，`EXC_RETURN` 编码决定返回到 Thread/Handler、使用 MSP/PSP，以及是否恢复浮点扩展栈帧。

**工程用途**：RTOS 任务通常用 PSP，PendSV/SVC/中断用 MSP。这样单个任务栈溢出不直接占用异常栈，但仍应配合 MPU 或水位检测。

**记忆**：**MSP 给复位与异常，PSP 给线程与任务。**

**手册依据**：PM0214 §2.1.3、§2.3.7；CMSIS Core Register Access。[^pm0214][^cmsis-reg]

## Q5：xPSR 中最值得面试时说明的位是什么？

**回答**：APSR 保存 N/Z/C/V/Q 等运算标志；IPSR 保存当前异常号，0 表示 Thread mode；EPSR 保存执行状态，其中 T 位必须为 1，表示 Thumb 状态。Cortex-M 只支持 Thumb 指令集状态。函数指针/异常向量中的入口地址 bit[0] 用来表示 Thumb 状态；跳转到 bit[0]=0 的入口会产生 fault。

**工程检查**：Bootloader 校验 App 复位向量时，不只检查地址落在 Flash，还应检查 `(reset_vector & 1U) != 0`。

**记忆**：**IPSR 看是否在中断，T 位说明 Cortex-M 只跑 Thumb。**

**手册依据**：PM0214 §2.1.3、§2.3.3；Armv7-M ARM 的 EPSR/T 位和异常向量定义。[^pm0214][^armv7m]

## Q6：Cortex-M 的 4GB 地址空间怎样分区？**
>思考：启动流程

**回答**：Armv7-M 定义统一 32 位地址空间及默认内存属性：代码区从 `0x00000000` 开始，SRAM 区从 `0x20000000` 开始，外设区从 `0x40000000` 开始，系统控制空间位于 `0xE0000000` 附近。具体哪个物理存储器映射到这些范围，由芯片实现决定。STM32F407 的主 Flash 位于 `0x08000000`，SRAM 从 `0x20000000` 开始，启动时还会把选中的启动存储器别名映射到 `0x00000000`。

**误区**：Cortex-M 没有传统 MMU 虚拟地址翻译，并不等于芯片不能做地址别名或重映射。

**记忆**：**Arm 定大区，芯片定具体块；F4 Flash 在 0x08000000，SRAM 在 0x20000000。**

**手册依据**：Armv7-M ARM，System address map；RM0090 §2.3、§2.4。[^armv7m][^rm0090]

## Q7：Cortex-M 是大端还是小端？未对齐访问一定可以吗？

**回答**：Cortex-M3/M4/M7 通常实现小端数据访问，架构也定义可配置/实现相关的端序能力，必须以芯片实现为准。Armv7-M 对部分普通 `LDR/STR` 允许未对齐访问，但 `LDM/STM`、`PUSH/POP`、独占访问等有更严格要求；`SCB->CCR.UNALIGN_TRP=1` 可让未对齐访问触发 UsageFault。Device/Strongly-ordered 类型区域也不能按普通 SRAM 的规则推断。

**工程建议**：协议字节流用显式移位或 `memcpy` 解析，不要把未对齐缓冲区强转成结构体指针。

**记忆**：**小端是常见实现，未对齐不是通行证；外设区尤其不能乱强转。**

**手册依据**：Armv7-M ARM，Memory model 与 load/store 对齐规则；PM0214 §2.2.5、`CCR.UNALIGN_TRP`。[^armv7m][^pm0214]

## Q8：MPU 能解决什么问题，不能解决什么问题？

**回答**：MPU 通过 `MPU_RNR/RBAR/RASR`（Armv7-M）为区域定义访问权限、XN、内存类型以及 Shareable/Cacheable/Bufferable 属性。它能阻止 CPU 的非法读写/执行并产生 MemManage fault，也可把 SRAM 标记为 XN、把任务区隔离。MPU 不是 MMU，不做地址翻译；它通常只约束处理器访问，DMA 等其他总线主设备不一定受它过滤。

**配置要点**：先禁用 MPU，再配置区域与属性并启用所需 MemManage fault，最后启用 MPU；在禁用、配置和重新启用的边界，严格执行对应手册要求的 `DMB/DSB/ISB` 序列。区域覆盖和优先级要按相应内核手册核对。

**记忆**：**MPU 管权限和属性，不做虚拟地址；管 CPU，不当然管 DMA。**

**手册依据**：PM0214 §4.5；AN4838；AN5156 §6.12。[^pm0214][^an4838][^an5156]

## Q9：Cortex-M4F 的 FPU 为什么“有硬件却没生效”？

**回答**：FPU 必须同时满足：芯片实现 FPU、编译目标启用正确的浮点指令与 ABI、运行时允许访问 CP10/CP11。Cortex-M4 常通过 `SCB->CPACR` 的 CP10/CP11 字段开放访问，并用 `DSB/ISB` 使配置生效。若不同目标文件混用不兼容的 hard-float/soft-float ABI，链接或调用约定会出错。异常时的浮点上下文还涉及 `FPCCR` 的自动/惰性保存机制。

**记忆**：**FPU 生效要三件事：硅上有、编译用、CPACR 开。**

**手册依据**：PM0214 §4.6；Cortex-M4 TRM 的 FPU 和 lazy stacking。[^pm0214][^cm4trm]

---

# 2. 中断与异常系统

## Q10：NVIC 的“抢占优先级”和“子优先级”到底是什么？

**回答**：NVIC 每个可配置异常有优先级字段，数值越小，逻辑优先级越高。`SCB->AIRCR.PRIGROUP` 把芯片实际实现的优先级位分成 group priority（决定能否抢占）和 subpriority（多个异常同时 pending 时决定先服务谁）。CMSIS 用 `NVIC_SetPriorityGrouping()`、`NVIC_EncodePriority()` 屏蔽具体位宽差异。

**默认值**：Armv7-M 架构中 `PRIGROUP` 复位为 0，但芯片库可能随后改写；STM32 工程必须看 `SystemInit/HAL_Init` 和 `AIRCR` 实值，不能背“默认组 2/4”。

**记忆**：**组优先级决定能不能打断，子优先级决定同时等待时谁先上。**

**手册依据**：PM0214 §4.2、`AIRCR.PRIGROUP`；CMSIS NVIC API。[^pm0214][^cmsis-nvic]

## Q11：高抢占优先级能否打断低优先级 ISR？同抢占级呢？同pending*

**回答**：当前未被屏蔽且新异常的 group priority 更高时，可以抢占。group priority 相同不能彼此抢占，即使 subpriority 不同；subpriority 只在处理器要从多个 pending 异常中选择下一个时生效。NMI 和 HardFault 有固定的特殊优先级关系，不受普通可配置优先级规则完全支配。

**常见坑**：把“优先级寄存器数值大”说成“优先级高”是反的；写入的低有效位还可能因芯片未实现而读回 0。

**记忆**：**数值越小越急；同组不抢占，子优先级只排队。**

**手册依据**：PM0214 §2.3.6、§4.2；Cortex-M4 Generic User Guide，Exception model。[^pm0214][^cm4dug]

## Q12：NVIC 的 enable、pending、active 分别表示什么？***

**回答**：enable 决定外部中断是否可被处理；pending 表示中断请求已挂起；active 表示处理器正在服务该异常。它们是不同状态，禁用 IRQ 不等于清除 pending。CMSIS 分别提供 `NVIC_EnableIRQ`、`NVIC_Set/ClearPendingIRQ`、`NVIC_GetActive`。许多外设还有自己的状态标志和中断使能位，NVIC 清 pending 不能替代清外设源标志。

**工程顺序**：配置外设并清旧标志 → 设 NVIC 优先级 → 清 NVIC pending → enable 外设请求和 NVIC。

**记忆**：**外设产生源，NVIC 管门和排队；清队列不等于清源头。**

**手册依据**：CMSIS NVIC API；PM0214 §4.3 NVIC 寄存器。[^cmsis-nvic][^pm0214]

## Q13：异常进入时硬件自动压栈什么？*

**回答**：基本异常栈帧包含 `R0-R3、R12、LR、PC、xPSR`。处理器更新 LR 为特殊的 `EXC_RETURN`，取向量表入口并进入 Handler mode。根据 FPU 状态和惰性压栈配置，可能还有扩展浮点栈帧。软件编译器生成的 ISR 序言还可能保存其他被调用者保存寄存器，这部分不是硬件基本栈帧。

**调试价值**：HardFault 中定位异常前 PC，应解析“异常使用 MSP 还是 PSP”，再读取基本栈帧中的 PC/LR/xPSR。

**记忆**：**硬件先保存 8 个基本现场，LR 变成 EXC_RETURN，向量表给新 PC。**

**手册依据**：Armv7-M ARM，Exception entry/return；PM0214 §2.3.7。[^armv7m][^pm0214]

## Q14：Tail-chaining 和 late arrival 为什么能降低中断延迟？

**回答**：若一个异常退出时已经有合格的 pending 异常，tail-chaining 可直接转入下一个 Handler，省去完整出栈再压栈。若异常入栈过程中到达了更高优先级异常，late arrival 允许处理器优先进入更高优先级 Handler，而不必先执行原目标 Handler。具体周期数依实现、存储器等待和总线状态而变化。

**记忆**：**尾链省一次出入栈，迟到的更高优先级可在入口阶段插队。**

**手册依据**：Cortex-M4 Generic User Guide，Exception model；Cortex-M4 TRM，NVIC functional description。[^cm4dug][^cm4trm]

## Q15：STM32 EXTI 的“中断模式”和“事件模式”有什么区别？

**回答**：这是 STM32 外设概念，不是 Cortex-M 通用概念。以 STM32F407 为例，边沿检测由 `EXTI_RTSR/FTSR` 配置；`EXTI_IMR` 允许中断请求送往 NVIC；`EXTI_EMR` 允许事件请求。中断路径让 CPU 进入 ISR；事件路径可参与唤醒或触发芯片内部事件机制，不等价于执行 ISR。pending 状态通过 `EXTI_PR` 按手册规定清除。

**记忆**：**IMR 路径找 NVIC/ISR，EMR 路径发事件；两者可同时开。**

**手册依据**：RM0090，External interrupt/event controller (EXTI) 章节。[^rm0090]

## Q16：ISR 中“不能做什么”？

**回答**：架构没有“禁止调用某个 C 函数”的清单，真正约束是确定性、可重入性和上下文。ISR 应避免：无界阻塞；等待只能由较低/相同优先级中断推进的条件；长循环和大块复制；不可重入库；普通互斥锁；大栈对象；在 M7 上遗漏 DMA/Cache 同步。共享数据要用合适的原子操作、临界区或无锁队列。

**面试追问**：`printf/malloc` 不是天然绝对禁止，而是通常耗时不可控、可能持锁、不可重入，因此不适合实时 ISR。

**记忆**：**ISR 只做采样、清源、入队、通知；重活下放线程。**

**手册依据**：Armv7-M 异常模型给出执行上下文；具体库的可重入性必须查工具链库手册。[^armv7m]

## Q17：为什么 ISR 中调用 `delay()` 经常死锁？

**回答**：若 `delay()` 等待 SysTick 递增，而当前 ISR 的抢占优先级高于或等于 SysTick，SysTick 不能执行，等待条件永远不变化。即便 SysTick 能抢占，长时间占用 ISR 仍会放大其他中断延迟。忙等型短延时不会因 tick 停止而必然死锁，但仍阻塞处理器并破坏实时性。

**解决**：ISR 记录时间戳/设置状态/投递事件，主循环或任务根据 deadline 执行；硬实时短脉冲优先用定时器输出比较/PWM。

**记忆**：**中断里等 tick，tick 又进不来，就是自己等自己。**

**手册依据**：PM0214 的异常抢占规则和 SysTick 章节；CMSIS SysTick API。[^pm0214][^cmsis-systick]

## Q18：ISR 修改、主循环读取的变量为什么常用 `volatile`？

**回答**：`volatile` 告诉 C 编译器，对该对象的每次读写都是可观察副作用，不能把循环中的读取长期寄存器化或删除。但它不保证复合表达式原子，不建立临界区，也不自动产生 `DMB/DSB` 或做 Cache 维护。单核 Cortex-M 上，一个自然对齐、架构支持宽度的单次 load/store 通常是单拷贝原子，但 `counter++` 是读-改-写，仍会竞争。

```c
static volatile uint32_t flag;

void IRQ_Handler(void) { flag = 1U; }
// 主循环会重新读取 flag，但更复杂共享状态仍需同步设计。
```

**记忆**：**volatile 解决“编译器别藏”，不解决“多人同时改”。**

**手册依据**：Arm Compiler Language Reference 的 volatile 语义；Armv7-M ARM 的访问原子性和内存顺序。[^armclang][^armv7m]

## Q19：PRIMASK、BASEPRI、FAULTMASK 有什么区别？

**回答**：`PRIMASK=1` 阻止所有可配置优先级异常，NMI 和 HardFault 仍可激活；`BASEPRI!=0` 屏蔽优先级数值不高于阈值紧急程度的异常，即屏蔽“同等或更低逻辑优先级”的可配置异常；`FAULTMASK=1` 屏蔽除 NMI 外的所有异常。Cortex-M0/M0+ 没有 BASEPRI/FAULTMASK。临界区通常优先用 BASEPRI 保留最高优先级响应，而不是长时间全局关中断。

**记忆**：**PRIMASK 一刀切普通中断；BASEPRI 留急诊；FAULTMASK 几乎全封。**

**手册依据**：CMSIS Core Register Access；PM0214 §2.2.1。[^cmsis-reg][^pm0214]

## Q20：修改中断屏蔽或系统控制寄存器后为什么常见 `DSB/ISB`？

**回答**：`DMB` 保证显式内存访问的观察顺序；`DSB` 等待此前显式内存访问完成；`ISB` 刷新流水线，使后续指令在新上下文下重新取指。修改 MPU、Cache、向量/系统控制、进入低功耗等场景，手册常要求特定 barrier 序列。barrier 不是 Cache clean/invalidate，也不是 C 语言层面的互斥锁。

**记忆**：**DMB 排顺序，DSB 等完成，ISB 刷流水；三者都不替你刷数据 Cache。**

**手册依据**：CMSIS CPU Intrinsics；Armv7-M ARM，Barrier instructions。[^cmsis-intrin][^armv7m]

---

# 3. 时钟、复位与低功耗

> 本章的 HSI/HSE/PLL/APB 数值是 **STM32F407 实例**。Cortex-M 内核本身不规定这些振荡器和时钟树。

## Q21：HSI、HSE、LSI、LSE 分别是什么？

**回答（STM32F407）**：HSI 是片内 16 MHz 高速 RC；HSE 是外部高速晶体/陶瓷谐振器或外部时钟输入；LSI 是片内低速 RC，频率精度较低，可供 IWDG/RTC 等；LSE 通常接 32.768 kHz 外部晶体，为 RTC 提供更高精度低速时钟。具体频率范围、精度、启动时间和温漂必须查 DS8626 Electrical characteristics，不能只看 RM。

**选择原则**：低成本/快速启动用内部 RC；通信精度和主频稳定性常用 HSE；独立看门狗偏向 LSI；长期计时偏向 LSE。

**记忆**：**H 是高速、L 是低速；I 是内部、E 是外部。**

**手册依据**：RM0090 RCC 章节；DS8626，Clock characteristics。[^rm0090][^ds8626]

## Q22：如何用 8 MHz HSE 配到 STM32F407 的 168 MHz，而不是机械背“72 MHz”？

**回答**：STM32F407 主 PLL 关系为：

$$f_{VCOin}=f_{PLLsource}/PLLM$$

$$f_{VCOout}=f_{VCOin}\times PLLN$$

$$f_{SYSCLK}=f_{VCOout}/PLLP$$

例如 HSE=8 MHz，可选 `PLLM=8, PLLN=336, PLLP=2`，得到 VCO 输入 1 MHz、VCO 输出 336 MHz、SYSCLK 168 MHz；还必须选择合法的 `PLLQ` 以满足需要的 48 MHz 域。所有 VCO、总线、Flash 延迟和电压范围须同时符合 RM0090/DS8626。

**纠错**：8→72 MHz 是常见 STM32F1 场景，不是 Cortex-M 或 STM32F407 的通用目标。

**记忆**：**先除到合法 VCO 输入，再乘到 VCO，最后用 PLLP 得 SYSCLK。**

**手册依据**：RM0090 §7 RCC、`RCC_PLLCFGR`；DS8626 PLL electrical characteristics。[^rm0090][^ds8626]

## Q23：AHB、APB1、APB2 为什么要分频，限制为何不同？

**回答**：这是 SoC 总线和外设实现限制。STM32F407 用 `RCC_CFGR.HPRE/PPRE1/PPRE2` 从 SYSCLK 生成 HCLK、PCLK1、PCLK2。不同 APB 域连接的外设电路、桥和时序目标不同，因此允许的最高频率不同；F407 在 168 MHz 工作点通常要求 HCLK≤168 MHz、PCLK2≤84 MHz、PCLK1≤42 MHz。不要把来源 PDF 中“36/72 MHz”套到 F407，那是其他器件配置。

**工程检查**：配置后读取 `RCC_CFGR`，结合时钟树重新计算 UART、Timer、ADC 等输入时钟。

**记忆**：**内核能跑多快，不代表每条外设总线都能跑同样快。**

**手册依据**：RM0090 RCC clock tree；DS8626 General operating conditions。[^rm0090][^ds8626]

## Q24：为什么 APB 分频后，定时器时钟可能是 PCLK 的 2 倍？

**回答**：对 STM32F407，多数连接 APB 的定时器遵循：若 APB prescaler=1，则定时器时钟 `TIMxCLK=PCLKx`；若 APB prescaler 不为 1，则 `TIMxCLK=2×PCLKx`。这是 STM32 时钟树的器件规则，不是 Cortex-M 规则。计算 PWM/计时错误一倍，通常就是漏了这一条。

**记忆**：**APB 不分频：TIM=PCLK；APB 分频：多数 TIM=2×PCLK。**

**手册依据**：RM0090 RCC clock tree 和 Timer clock；AN4013，Timer clock sources。[^rm0090][^an4013]

## Q25：外部晶体模式和外部时钟旁路模式有何区别？

**回答**：晶体模式由 MCU 内部振荡放大器配合外部晶体及负载电容起振；旁路模式由外部有源时钟源直接驱动 OSC_IN，并通过 `RCC_CR.HSEBYP` 选择。两种接法、电气振幅、引脚使用和启动特性不同，不能把有源时钟模块按无源晶体接法使用。

**记忆**：**晶体靠片内振荡器起振；旁路是外部已经给你时钟波形。**

**手册依据**：RM0090 RCC；DS8626 HSE oscillator characteristics。[^rm0090][^ds8626]

## Q26：进入 Stop 模式后哪些时钟停止，唤醒后为什么要重配 PLL？

**回答（STM32F407）**：Stop 模式保持 SRAM/寄存器内容，但关闭主调压器的正常运行状态并停止 HSI/HSE/PLL 等高速时钟域，具体取决于 Stop 配置。退出 Stop 后系统先以手册规定的内部时钟运行，若应用需要原 HSE+PLL 主频，软件必须等待 HSE/PLL ready，再按安全顺序切回 SYSCLK。RTC/LSE/LSI 等备份域时钟是否继续取决于其独立配置。

**工程流程**：保存业务状态 → 清唤醒标志 → 配 PWR/`SCB.SLEEPDEEP` → WFI/WFE → 唤醒后恢复系统时钟和外设时基。

**记忆**：**Stop 保数据但停高速钟；醒来先站稳内部钟，再重锁 PLL。**

**手册依据**：RM0090 PWR 与 RCC 章节；DS8626 Low-power mode characteristics。[^rm0090][^ds8626]

## Q27：为什么配置 GPIO 前要开启 RCC 时钟？

**回答**：STM32 用时钟门控降低动态功耗。GPIOA 配置寄存器位于 AHB1 外设域，需设置 `RCC_AHB1ENR.GPIOAEN` 才保证接口时钟可用。不同器件在关钟时对读写的确切行为必须看 RM；工程上不能依赖“写了但没反应”的偶然表现。启用后如立即访问，按芯片手册和总线要求处理必要的读回/延迟。

**记忆**：**外设寄存器是电路，时钟没送到，状态机就不能正常工作。**

**手册依据**：RM0090 RCC enable registers 与 GPIO 章节。[^rm0090]

## Q28：提高主频时，为什么必须同时设置电压等级和 Flash wait states？

**回答**：CPU/HCLK 提高后，片上 Flash 的读取延迟可能跟不上。STM32F407 需根据供电电压和 HCLK，在 `FLASH_ACR.LATENCY` 选择等待周期，并按需要配置 Flash 接口的 prefetch、instruction cache、data cache；这些是 STM32 Flash 加速机制，不是 Cortex-M7 的 L1 Cache。升频前还要满足电源调压器/电压范围要求，通常先提高供电能力和 Flash latency，再切高频。

**记忆**：**主频不是只改 PLL：电压撑得住，Flash 等得够，才能切过去。**

**手册依据**：RM0090 Embedded Flash memory interface 与 PWR；DS8626 AC characteristics。[^rm0090][^ds8626]

## Q29：WFI、WFE、SEV 和 `SLEEPDEEP` 是什么关系？

**回答**：`WFI` 等待合格中断，`WFE` 等待事件，`SEV` 产生事件。`SCB->SCR.SLEEPDEEP=0` 时进入普通 Sleep；置 1 时，具体 MCU 电源控制逻辑可进入 deep sleep 对应的 Stop/Standby。WFE 受事件寄存器状态影响，常见的 `SEV; WFE; WFE` 序列用于先清理旧事件再真正等待，但是否适用要结合并发设计。

**记忆**：**WFI 等中断，WFE 等事件；SLEEPDEEP 决定睡浅还是睡深。**

**手册依据**：Armv7-M ARM 的 WFI/WFE/SEV；PM0214 `SCR`；RM0090 PWR。[^armv7m][^pm0214][^rm0090]

---

# 4. 存储、链接与启动流程

## Q30：STM32F407 的 BOOT0/BOOT1 如何选择启动空间？

**回答**：STM32F405/407 在复位时根据 BOOT 引脚选择把主 Flash、System memory 或 embedded SRAM 映射到启动别名区 `0x00000000`。具体组合见 RM0090 的 Boot configuration 表；不同 STM32 系列已改用不同 BOOT option bytes/模式，不能跨系列背一张表。System memory 中是 ST 在生产时写入的 ROM bootloader，支持接口还要查当前版本 AN2606 对应器件条目。

**记忆**：**BOOT 选择谁映射到 0 地址；真正物理 Flash 仍在 0x08000000。**

**手册依据**：RM0090 §2.4 Boot configuration；AN2606 对 STM32F405/407 的器件条目。[^rm0090][^an2606]

## Q31：中断向量表前两个字是什么？

**回答**：向量表偏移 0 的字是初始 MSP 值；偏移 4 的字是 Reset Handler 入口。后续依次是 NMI、HardFault 等系统异常和器件中断。处理器复位时从向量表读取 MSP 和 PC。复位向量入口必须表示 Thumb 状态。

**App 合法性最低检查**：MSP 落在该芯片有效、允许作为栈的 RAM 范围且满足 ABI/实现对齐；Reset vector bit[0]=1，去掉 bit[0] 后落在允许执行区域；镜像长度和完整性校验通过。

**记忆**：**第 0 字装 MSP，第 1 字装复位 PC。**

**手册依据**：Armv7-M ARM，Reset behavior/vector table；PM0214 §2.3.4。[^armv7m][^pm0214]

## Q32：从复位到 `main()`，哪些是硬件行为，哪些是软件行为？

**回答**：硬件负责进入复位状态、从向量表加载 MSP 和 Reset vector、开始执行 Reset Handler。之后属于启动软件和 C 运行库：低级系统初始化、复制有初值数据、清零零初始化区、初始化 C/C++ 运行时，最后调用 `main()`。CMSIS 规定启动文件提供向量表、MSP、弱中断处理和 Reset Handler，并通常调用 `SystemInit()` 后转入 C/C++ runtime。

**关键边界**：`.data/.bss` 不是 Cortex-M 硬件自动初始化的。

**记忆**：**硬件只把栈和入口交给你；C 世界的内存环境由启动代码搭起来。**

**手册依据**：CMSIS Startup File；Armv7-M reset model。[^cmsis-startup][^armv7m]

## Q33：`startup_stm32xxxx.s` 通常包含什么？

**回答**：典型内容包括栈/堆区域声明或链接符号、向量表、Reset Handler、系统异常和外设 IRQ 的 weak 默认实现。Reset Handler 常调用 `SystemInit()` 并转入工具链运行库入口。不同工具链可能由启动汇编自己复制 `.data`/清 `.bss`，也可能委托 `__main`、`_start` 或类似 runtime；必须读当前工程启动文件和链接脚本。

**记忆**：**启动文件的四件事：栈、向量、复位入口、默认异常入口。**

**手册依据**：CMSIS Startup File `startup_<Device>.c/.S`；CMSIS System and Clock Configuration。[^cmsis-startup][^cmsis]

## Q34：`.text/.rodata/.data/.bss` 分别在哪里，为什么 `.data` 有两个地址？

**回答**：`.text` 是代码，`.rodata` 是只读常量，通常执行/读取于 Flash；`.data` 是有非零初值的可写全局/静态对象，初值镜像存于 Flash，运行地址在 RAM；`.bss` 是零初始化或未显式初始化的全局/静态对象，运行时在 RAM 清零，镜像不必保存一份全零数据。局部自动变量通常在栈，动态分配来自堆，具体由 ABI/运行库和优化决定。

**记忆**：**data：Flash 带初值、RAM 中运行；bss：RAM 中清零。**

**手册依据**：CMSIS Startup File；Arm Compiler Linker User Guide 的 execution/load region。[^cmsis-startup][^armlink]

## Q35：`0x08000000` 和 `0x20000000` 为什么常见？

**回答**：`0x20000000` 是 Armv7-M 默认 SRAM 大区起点；`0x08000000` 是 STM32 为片上主 Flash 选择的器件映射。Arm 统一大区有利于内存属性和软件生态一致，但具体物理块大小、别名、总线可达性都由器件决定。STM32F407 还存在 CCM data RAM 等特殊区域，其可被哪些总线主设备访问必须查 RM0090 总线矩阵，不能仅凭“也是 RAM”推断。

**记忆**：**0x200…是架构 SRAM 大区；0x080…是 STM32 主 Flash 映射。**

**手册依据**：Armv7-M System address map；RM0090 Memory map/Bus matrix。[^armv7m][^rm0090]

## Q36：VTOR 是什么，Bootloader 跳 App 为什么常要改它？

**回答**：`SCB->VTOR` 指定向量表基址。Bootloader 与 App 使用不同向量表时，App 运行前必须确保 VTOR 指向 App 向量表，否则异常仍会跳到 Bootloader 的表。VTOR 基址需满足对应内核/实现的对齐要求；对齐不是固定背一个数，而是与支持的异常数量和手册要求相关。写 VTOR 后按手册使用 `DSB/ISB`。

**记忆**：**PC 跳到 App 只换了代码入口；VTOR 才换了中断入口。**

**手册依据**：PM0214 `SCB_VTOR`；Armv7-M Vector Table Offset Register。[^pm0214][^armv7m]

## Q37：链接脚本/Scatter file 到底解决什么问题？

**回答**：它把输入目标文件的 section 组织成最终镜像，并定义每个输出段的运行地址、装载地址、大小、对齐和符号。例如定义 Bootloader/App 分区、`.isr_vector` 固定位置、`.data` 的 Flash LMA 与 RAM VMA、NOLOAD `.bss`、栈堆边界、RAM 函数区。它决定“二进制放哪里、运行时从哪里取”。

**工程检查**：每次改分区都看 `.map` 文件，验证段没有跨边界、入口/向量地址正确、RAM/Flash 余量真实。

**记忆**：**编译器造零件，链接脚本决定零件摆进哪块 Flash/RAM。**

**手册依据**：Arm Linker User Guide；CMSIS startup/linker template。[^armlink][^cmsis-startup]

## Q38：VMA 与 LMA 的区别是什么？

**回答**：VMA（执行/运行地址）是程序运行时访问该段的地址；LMA（装载地址）是镜像中保存初始内容的位置。`.data` 典型为 LMA=Flash、VMA=RAM，启动时复制；普通 `.text` 常为 LMA=VMA=Flash；RAM 函数也常为 LMA=Flash、VMA=RAM。

**记忆**：**LMA 问“镜像存哪”，VMA 问“运行在哪”。**

**手册依据**：Arm Linker User Guide，Load regions and execution regions。[^armlink]

## Q39：怎样让函数在 RAM 中运行？哪些场景需要？

**回答**：把函数放入专用 section，在链接脚本中给它分配 Flash 装载地址和 RAM 执行地址，启动或使用前复制到 RAM；保证目标 RAM 可执行，并在 M7 上处理 I-Cache/D-Cache 和屏障。常见用途是同 Bank Flash 擦写期间必须执行的代码、极低延迟热点、Flash 等待周期敏感代码。STM32F407 的 CCM RAM 能否取指、DMA 能否访问必须按 RM0090 的总线连接分别判断。

**记忆**：**特殊 section + 双地址 + 启动复制；还要确认那块 RAM 真能取指。**

**手册依据**：RM0090 Flash/Bus matrix；Arm Linker User Guide。[^rm0090][^armlink]

## Q40：怎样判断栈溢出？

**回答**：链接阶段只能保证静态预留大小，不证明运行峰值足够。常用办法：栈区填充固定模式并统计 high-water mark；在边界设置 MPU no-access guard；开启 RTOS 栈检查；HardFault 中检查 MSP/PSP 是否落界；结合编译器 stack-usage 文件和最大中断嵌套估算。异常栈还要计入硬件基本/浮点栈帧和 ISR 调用链。

**记忆**：**静态算调用深度，运行看水位，边界用 MPU 抓越界。**

**手册依据**：PM0214 exception stack frame/MPU；AAPCS32 stack constraints。[^pm0214][^aapcs]

## Q41：AAPCS 为什么要求公共接口处 SP 8 字节对齐？

**回答**：AAPCS32 规定栈在公共接口处满足双字对齐，以保证需要 8 字节对齐的数据和调用约定正确。Cortex-M 异常入口还可通过 `CCR.STKALIGN` 保持 8 字节栈对齐，并在栈帧 xPSR 中记录是否插入对齐填充。手写汇编、上下文切换和 Bootloader 设置 MSP 时必须保持 ABI 约束。

**记忆**：**栈始终至少字对齐；跨函数公共接口按 8 字节对齐。**

**手册依据**：AAPCS32 §6.2.1；Armv7-M `CCR.STKALIGN` 与 exception entry。[^aapcs][^armv7m]

---

# 5. Bootloader、IAP 与可靠升级

## Q42：Bootloader/App/参数区应如何分区？

**回答**：没有固定“Bootloader 必须 16/32 KB”的通用值。分区必须由 Flash 擦除粒度、Bootloader 实际链接尺寸、向量表对齐、App 最大尺寸、升级策略、元数据/参数磨损和回滚需求共同确定。STM32F407 各 sector 大小不一致，分区边界应落在可独立擦除的 sector 上。

**交付物**：链接脚本、分区表、镜像头格式、sector 擦除表、量产烧录配置必须使用同一份地址定义。

**记忆**：**先看擦除粒度和镜像上限，再画分区；不要先拍脑袋写 32 KB。**

**手册依据**：RM0090 Flash memory organization；DS8626 Memory organization。[^rm0090][^ds8626]

## Q43：Bootloader 如何判断进入升级还是启动 App？

**回答**：常见输入包括升级请求标志、按键/引脚、通信握手、App 元数据状态和镜像验证结果。可靠逻辑不是“有向量就跳”，而是：读取不可歧义的状态记录 → 检查镜像边界与头 → 校验完整性/真实性 → 检查版本策略 → 决定启动、恢复或升级。状态记录要有 magic、格式版本、序列号/单调计数和校验，并能识别写一半。

**记忆**：**启动决策看状态机和镜像证据，不看一个裸 flag。**

**手册依据**：AN4657 IAP flow；AN5156 Secure boot/SFU。[^an4657][^an5156]

## Q44：从 Bootloader 跳 App 前要做哪些准备？

**回答**：先停止产生中断/DMA 的外设和 SysTick；按系统设计禁用相关 IRQ、清外设源与 NVIC pending；把时钟/电源/外设恢复到 App 明确约定的状态，或让 App 能接管现状；设置 `VTOR`；恢复期望的 `CONTROL/PRIMASK/BASEPRI`；从 App 向量表加载 MSP；调用合法的 Thumb Reset vector。是否需要全量 RCC deinit 取决于 Bootloader-App 接口约定，不是 Arm 架构硬规定。

**更稳原则**：能用系统复位进入新镜像时，通常比带着未知外设状态直接跳转更接近真实复位环境。

**记忆**：**停源、清挂起、换向量、换栈、跳复位入口。**

**手册依据**：PM0214 NVIC/VTOR/MSP；CMSIS NVIC API；AN4657。[^pm0214][^cmsis-nvic][^an4657]

## Q45：一个更严格的最小跳转函数应检查什么？

```c
/* GCC/Clang 示意；其他工具链用等价汇编实现。参数按 AAPCS 位于 r0/r1。 */
__attribute__((naked, noreturn))
static void branch_to_image(uint32_t new_msp, uint32_t reset_v)
{
    __asm volatile (
        "movs r2, #0    \n"
        "msr control,r2 \n"
        "msr msp,r0     \n"
        "isb            \n"
        "bx r1          \n");
}

bool jump_to_app(uint32_t base,
                 uint32_t ram_lo, uint32_t ram_hi_exclusive,
                 uint32_t exec_lo, uint32_t exec_hi)
{
    const uint32_t new_msp = *(const uint32_t *)(base + 0U);
    const uint32_t reset_v = *(const uint32_t *)(base + 4U);
    const uint32_t reset_a = reset_v & ~1UL;

    if ((new_msp < ram_lo) || (new_msp >= ram_hi_exclusive) ||
        ((new_msp & 0x7U) != 0U) ||
        ((reset_v & 1U) == 0U) ||
        (reset_a < exec_lo) || (reset_a >= exec_hi)) {
        return false;
    }

    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;
    SCB->VTOR = base;
    __DSB();
    __ISB();
    branch_to_image(new_msp, reset_v); /* 见下方说明 */
}
```

**边界**：代码只展示 core 切换骨架。`branch_to_image()` 应使用工具链对应的极小汇编 trampoline：按双方约定设置 `CONTROL`，写入新 MSP，执行 `ISB` 后用 `BX reset_v` 跳转；写 MSP 后不得再执行依赖旧栈的普通 C 代码。IRQ 数量、NVIC 清理、外设/DMA/Cache/时钟清理、RAM 区间、镜像认证都必须按目标芯片和产品实现。上例保持 PRIMASK 屏蔽状态，App 的 Reset Handler 必须按双方启动契约在安全时点恢复中断；直接跳转并不具有硬件 Reset 的全部副作用。

**记忆**：**MSP 查 RAM 与 8 字节对齐；入口查范围与 Thumb bit。**

**手册依据**：Armv7-M reset/vector semantics；PM0214 MSP/VTOR/CONTROL；AAPCS32 stack alignment。[^armv7m][^pm0214][^aapcs]

## Q46：为什么仅 `__disable_irq()` 仍不等于“中断处理干净了”？

**回答**：`__disable_irq()` 设置 PRIMASK，只阻止可配置异常激活；它不停止外设继续置位，不清 NVIC pending，不影响 NMI，也不自动复位 SysTick/DMA。若 App 后来重新开中断，Bootloader 留下的 pending 可能立即触发。因此应从“中断源—外设状态—NVIC pending—优先级/向量—全局屏蔽”整条链路处理。

**记忆**：**关总闸只是不让它现在进门；源头和排队记录还在。**

**手册依据**：CMSIS Core Register/NVIC API；PM0214 NVIC。[^cmsis-reg][^cmsis-nvic][^pm0214]

## Q47：片内 Flash 擦写有哪些硬约束？

**回答（STM32F407）**：擦除按 sector，编程按手册支持的 program size，并受电压范围约束；操作前解锁，清/检查错误标志，等待 `FLASH_SR.BSY` 清除，设置 `FLASH_CR` 的操作位，完成后验证并重新锁定。同一 Flash bank 擦写时的取指/读取限制必须看器件手册；中断向量或 ISR 若仍从受影响区域取指，会破坏实时性甚至 fault。

**记忆**：**Flash 先擦后写、按粒度、等 BUSY、查错误、写后校验。**

**手册依据**：RM0090 Embedded Flash memory interface，`FLASH_SR/CR`。[^rm0090]

## Q48：升级时突然断电，怎样保证不变砖？

**回答**：不能覆盖唯一可启动镜像后再祈祷。可靠流程是把新镜像写入非活动区或外部 staging 区；分块写入并校验；最后以原子性更强、可恢复的方式提交“候选有效”状态；Bootloader 重启后只启动完整验证通过的镜像。元数据采用双副本、序列号和校验，状态转移只允许单向推进，未完成状态自动回旧镜像/恢复模式。

**记忆**：**先完整写备用，再验证，最后提交；提交之前旧版本始终可启动。**

**手册依据**：AN4657 的 IAP 分区/下载流程；AN5156 的 secure firmware update 原则。[^an4657][^an5156]

## Q49：CRC、MD5/SHA 和数字签名分别解决什么？

**回答**：CRC 适合检测随机传输/存储错误，不提供对恶意篡改的真实性保证；密码学哈希用于形成抗碰撞摘要，但单独哈希仍不能证明“是谁发布的”；数字签名用受保护的私钥签发、设备内可信公钥验证，可同时验证完整性与来源真实性。MD5 已不适合作为安全完整性方案。是否加密是另一个目标：机密性不等于真实性。

**记忆**：**CRC 防随机错，哈希做摘要，签名证明来源；加密只负责不让人看。**

**手册依据**：AN5156，Integrity/authentication/confidentiality 与 Secure boot。[^an5156]

## Q50：双 Bank 与逻辑 A/B 是一回事吗？

**回答**：不是。双 Bank 是 Flash 硬件组织，可能支持 bank swap、read-while-write 等；A/B 是软件升级策略，两个槽位也可以位于单 Bank 或外部存储。反过来，有双 Bank 也不会自动得到回滚状态机。必须分别确认：硬件能否并行读写、bank swap 如何生效、启动地址如何选择、两个镜像如何认证和提交。

**记忆**：**Bank 是硬件结构，A/B 是软件策略；两者能配合但不等价。**

**手册依据**：目标器件 Flash reference manual；AN5156 Secure firmware update。STM32F407 具体组织见 RM0090。[^rm0090][^an5156]

## Q51：如何做失败回滚与防降级？

**回答**：候选镜像先进入 trial 状态；Bootloader 启动它并开启启动确认期限；App 完成关键自检后写入 confirmed；若 watchdog/reset 次数超过阈值或未确认，则回滚。防降级需要受保护的单调版本计数或安全生命周期机制，并把版本字段包含在签名覆盖范围内；普通可擦写变量不能抵抗恶意回写。

**记忆**：**新版本先试运行，App 自证健康才转正；版本号也必须受签名和单调策略保护。**

**手册依据**：AN5156 Secure boot/SFU、root and chain of trust。[^an5156]

## Q52：一个可产品化的镜像头至少应有什么？

**回答**：建议包含 magic、格式版本、目标硬件/产品 ID、镜像版本、长度、入口/向量基址、构建标识、摘要算法与摘要、签名算法与签名定位、状态/序列号。Bootloader 必须先做溢出安全的边界检查，再读取可变长度字段；签名覆盖范围必须明确排除或规范化可变状态字段。

**记忆**：**先用头描述“是谁、给谁、多长、哪版、从哪进”，再用签名证明它是真的。**

**手册依据**：AN5156 Secure boot/SFU；AN4657 IAP image organization。[^an5156][^an4657]

---

# 6. 并发、原子操作、DMA 与 Cache

## Q53：单核 MCU 为什么也有并发问题？

**回答**：主循环、不同优先级 ISR、RTOS 任务和 DMA 会在时间上交错访问共享状态。即使只有一个 CPU，`counter++` 仍是 load-modify-store：主循环读取后被 ISR 打断，ISR 更新，主循环恢复后用旧值写回，就会丢失更新。DMA 还是独立总线主设备，可与 CPU 真正重叠访问内存。

**解决工具**：单写者设计、短临界区、BASEPRI、原子指令、消息队列、双缓冲和明确所有权。先定义并发模型，再选工具。

**记忆**：**单核不等于无并发；能被中断切开的多步操作就可能竞争。**

**手册依据**：Armv7-M exception model 和 exclusive access；AN4031 DMA architecture。[^armv7m][^an4031]

## Q54：32 位读写在 Cortex-M 上是否一定原子？

**回答**：不能只凭 C 类型下结论。必须满足架构支持的访问宽度、自然对齐、普通内存类型、编译器确实生成单条相应 load/store，并考虑目标总线/外设语义。单次原子访问也不等于复合读改写原子；`x |= mask` 仍可能被打断。64 位对象在 32 位内核上通常需要多次访问。

**记忆**：**一条对齐 load/store 才谈单拷贝原子；读改写一定另算。**

**手册依据**：Armv7-M ARM，Memory access atomicity/alignment。[^armv7m]

## Q55：LDREX/STREX 如何实现无锁原子更新？

**回答**：`LDREX` 建立本地独占监视并读取，软件计算新值，`STREX` 仅在独占状态仍有效时写入；返回 0 表示成功，非 0 必须重试。异常进入、其他主设备访问或实现相关事件可能使独占失败，因此循环必须允许失败，且不能在中间调用任意可能破坏监视状态的代码。`CLREX` 可显式清除独占状态。

```c
do {
    old = __LDREXW(&value);
} while (__STREXW(old + 1U, &value) != 0U);
```

该循环只说明更新的原子性；若它还承担“发布/获取”同步，屏障位置必须按共享数据协议设计，不能机械地在循环后加一个 `DMB`。

**记忆**：**LDREX 预约，STREX 尝试提交；失败是正常路径，循环重试。**

**手册依据**：Armv7-M ARM 的 exclusive access；CMSIS CPU Intrinsics。[^armv7m][^cmsis-intrin]

## Q56：禁中断、原子操作、内存屏障各解决什么？

**回答**：禁中断/BASEPRI 防止选定异常在临界区抢占；原子操作保证共享对象的更新不可被观察为中间状态；内存屏障约束访问顺序或完成性。三者不是替代关系。比如 ring buffer 发布索引前，既要保证数据先可见，又要保证索引更新本身正确；多核或 DMA 场景还可能需要 Cache 维护。

**记忆**：**关中断管调度，原子管不可分，屏障管先后；Cache 另有一套。**

**手册依据**：Armv7-M ARM；CMSIS Core Register/CPU Intrinsics。[^armv7m][^cmsis-reg][^cmsis-intrin]

## Q57：DMA 为什么能降低 CPU 负担？CPU 会完全不受影响吗？

**回答**：DMA 作为总线主设备，在外设与存储器之间搬运数据，CPU 不必为每个元素执行 load/store，只需配置并处理完成/半完成/错误事件。但 DMA 与 CPU 仍可能争用总线、存储器端口和外设，影响延迟；缓冲区所有权、对齐、Cache 一致性和错误处理仍由软件负责。

**记忆**：**DMA 省的是逐字节搬运，不是免费带宽，也不替你管理缓冲区。**

**手册依据**：AN4031；RM0090 DMA controller 章节。[^an4031][^rm0090]

## Q58：STM32F407 DMA 的 stream、channel、FIFO、double-buffer 分别是什么？

**回答**：F407 的 DMA 控制器包含多个 stream；每个 stream 通过 channel selection 连接到手册规定的外设请求映射，不能任意配。FIFO 可进行源/目标突发和宽度适配，并有阈值/错误状态；direct mode 绕过 FIFO。Double-buffer mode 让 `M0AR/M1AR` 两个内存地址交替工作，硬件用 `CT` 指示当前目标，适合连续采集。

**调试寄存器**：`DMA_SxCR/SxNDTR/SxPAR/SxM0AR/SxM1AR/SxFCR` 和对应 `LISR/HISR` 标志。

**记忆**：**channel 选请求，stream 执行传输，FIFO 做缓冲，双缓冲保连续。**

**手册依据**：RM0090 DMA；AN4031。[^rm0090][^an4031]

## Q59：Cortex-M7 开 D-Cache 后，DMA 为什么会读到旧数据或 CPU 看到旧数据？

**回答**：D-Cache 与主存可能存在两个版本。CPU 写 TX buffer 后若脏 Cache line 未 clean，DMA 从 RAM 读到旧值；DMA 写 RX buffer 后若 CPU Cache 中仍有旧 line，CPU 会读到旧值。发送前 clean，接收完成后 invalidate；按 CMSIS 要求把地址/范围覆盖到完整 Cache line，防止同一 line 上无关数据被误伤。另一策略是用 MPU 把 DMA 共享区设为 non-cacheable。

**记忆**：**CPU→DMA 前 clean；DMA→CPU 后 invalidate；缓冲区按 Cache line 隔离对齐。**

**手册依据**：CMSIS M7 D-Cache Functions；AN4839；PM0253 Cache/MPU。[^cmsis-cache][^an4839][^pm0253]

## Q60：`volatile` 或 `DMB` 能替代 Cache clean/invalidate 吗？

**回答**：不能。`volatile` 约束编译器访问；`DMB/DSB` 约束内存访问顺序/完成；clean 把脏 Cache line 写回更低层存储，invalidate 丢弃 Cache 中副本。它们作用层次不同。使用 DMA、双核共享内存或非一致性主设备时，必须按数据方向和所有权执行明确的 Cache 维护或使用 non-cacheable 区。

**记忆**：**volatile 管编译器，barrier 管顺序，cache maintenance 管数据副本。**

**手册依据**：CMSIS CPU Intrinsics/D-Cache Functions；AN4839。[^cmsis-intrin][^cmsis-cache][^an4839]

## Q61：DTCM/CCM 为什么经常不能直接给普通 DMA 用？

**回答**：能否访问取决于具体 SoC 总线拓扑，而不是内存名字。STM32F407 的 64 KB CCM data RAM 通过内核数据路径提供高性能，但普通 DMA 的总线路径不能访问它；不同 STM32H7 又可能允许特定 MDMA 访问 TCM。必须查看目标器件 bus matrix 图和 master/slave access table。

**记忆**：**“CPU 能访问”不推出“DMA 能访问”；先看总线矩阵的连线。**

**手册依据**：RM0090 System architecture/Bus matrix；AN4891 展示 M7/MDMA 的器件差异。[^rm0090][^an4891]

---

# 7. Fault、调试与系统可靠性

## Q62：Cortex-M 的 MemManage、BusFault、UsageFault、HardFault 如何区分？

**回答**：MemManage 通常来自 MPU/内存保护违规；BusFault 来自取指或数据总线访问错误；UsageFault 包括未定义指令、非法状态、除零陷阱、未对齐陷阱等；HardFault 处理不能由其他 fault 正常处理的故障，也可由可配置 fault 因未使能或优先级原因升级。`SCB->SHCSR` 控制可配置 fault 使能，`CFSR/HFSR` 给出原因。

**记忆**：**权限错看 MemManage，总线错看 BusFault，用法错看 UsageFault，升级/硬故障看 HardFault。**

**手册依据**：PM0214 Fault handling；Armv7-M `CFSR/HFSR/SHCSR`。[^pm0214][^armv7m]

## Q63：HardFault 现场应该记录哪些信息？

**回答**：至少记录异常栈帧 `R0-R3/R12/LR/PC/xPSR`，进入 handler 时的 MSP/PSP 和 `EXC_RETURN`，以及 `SCB->CFSR/HFSR/DFSR/AFSR/MMFAR/BFAR/SHCSR/ICSR`。只有相应 valid 位有效时才解释 MMFAR/BFAR。再用 ELF/map 反查 PC/LR，并保留固件 build ID，避免拿错版本符号表。

**记忆**：**先留栈帧，再留 SCB fault 寄存器，最后用同版本 ELF 定位。**

**手册依据**：PM0214 SCB fault registers/exception frame；Armv7-M Fault behavior。[^pm0214][^armv7m]

## Q64：精确 BusFault 与非精确 BusFault 有什么差异？

**回答**：精确数据总线错误能把栈中 PC 对应到引发 fault 的指令，`CFSR.BFSR.PRECISERR` 指示该类错误；非精确错误常来自缓冲写，处理器继续执行后才报告，栈中 PC 不一定是根因指令，对应 `IMPRECISERR`。调试时可按内核手册使用 `ACTLR.DISDEFWBUF`（若实现）牺牲性能使某些写错误更容易精确定位，但它不是所有 Cortex-M/所有错误的通用修复。

**记忆**：**PRECISERR 的 PC 值得信；IMPRECISERR 要向前追写操作。**

**手册依据**：PM0214 `CFSR.BFSR`；Cortex-M4 TRM `ACTLR`。[^pm0214][^cm4trm]

## Q65：为什么建议主动开启 MemManage/BusFault/UsageFault？

**回答**：若对应 configurable fault 未使能，许多错误会升级为 HardFault，丢失“由哪个专门 handler 分类处理”的机会。开发阶段可设置 `SCB->SHCSR` 的 `MEMFAULTENA/BUSFAULTENA/USGFAULTENA`，并根据需求设置 `CCR.DIV_0_TRP/UNALIGN_TRP`，让除零和未对齐错误尽早暴露。量产策略则应记录、进入安全态并受控复位。

**记忆**：**开发期让错误尽早、分类地炸出来；量产期要留证据并安全恢复。**

**手册依据**：PM0214 `SHCSR/CCR`；Armv7-M fault escalation。[^pm0214][^armv7m]

## Q66：DWT、ITM、SWO 分别能做什么？

**回答**：DWT 提供数据观察点、周期计数器和跟踪事件；ITM 提供软件刺激端口及事件输出；SWO 是部分器件/调试系统使用的单线跟踪输出路径。`DWT->CYCCNT` 常用于测量周期，但是否实现、是否在 sleep/debug 中计数以及解锁方式要查内核/芯片实现。它们属于调试与跟踪组件，不应把调试器连着时的行为当成量产时序。

**记忆**：**DWT 量周期/看数据，ITM 发跟踪，SWO 把跟踪送出去。**

**手册依据**：Cortex-M4 TRM Debug；PM0214 Debug。[^cm4trm][^pm0214]

## Q67：独立看门狗 IWDG 和窗口看门狗 WWDG 有何区别？

**回答（STM32F407）**：IWDG 由独立 LSI 时钟驱动，适合在主时钟失效时仍监督系统；一旦按手册启动，通常只能由复位停止。WWDG 由 APB 时钟域驱动，不仅要求在超时前刷新，还要求在允许窗口内刷新，过早/过晚都可发现流程异常，并可产生 early wakeup interrupt。两者具体计数公式和冻结行为见 RM0090/DBG 配置。

**正确喂狗**：只有完成关键任务健康检查后统一喂狗，不能由独立高优先级中断无条件喂。

**记忆**：**IWDG 独立保底，WWDG 还检查刷新时机。**

**手册依据**：RM0090 IWDG/WWDG；DS8626 LSI/IWDG characteristics。[^rm0090][^ds8626]

## Q68：复位后怎样知道上一次为什么重启？

**回答（STM32F407）**：尽早读取 `RCC_CSR` 中 BOR/PIN/POR/SFT/IWDG/WWDG/LPWR 等 reset flags，记录到保留 RAM/Flash 日志后，通过 `RMVF` 清除。还应结合应用自己的启动计数、Fault 记录和 watchdog 状态判断。注意上电复位可能同时造成多个标志组合，不能用“只判断一个 if”草率归因。

**记忆**：**启动第一件事先抄复位旗子，分析后再清；不要先初始化把证据抹掉。**

**手册依据**：RM0090 `RCC_CSR` 和 Reset chapter。[^rm0090]

## Q69：Brown-out/POR/PVD 分别解决什么？

**回答**：POR/PDR 是上电/掉电复位监督，保证电压越过器件阈值时保持或产生复位；BOR 在欠压条件下触发复位，阈值由器件/option bytes 决定；PVD 是可编程电压检测，可通过状态/EXTI 提前通知软件执行保存或安全关断。阈值、迟滞和最小脉宽必须查当前器件 datasheet。

**记忆**：**POR 管上电，BOR 管掉压复位，PVD 给软件一个提前处理机会。**

**手册依据**：RM0090 PWR/Reset；DS8626 electrical characteristics。[^rm0090][^ds8626]

---

# 8. 高频外设问题

## Q70：GPIO 输入、推挽、开漏、复用、模拟模式内部发生什么？

**回答（STM32F407）**：`GPIOx_MODER` 选择 input/output/alternate-function/analog；输出模式由 `OTYPER` 选择 push-pull/open-drain，`OSPEEDR` 控制输出驱动转换速度，`PUPDR` 选择内部上下拉，`AFRL/AFRH` 连接片上外设复用功能。Analog mode 关闭数字输入路径以降低功耗/噪声，常用于 ADC 和未用引脚策略。

**记忆**：**MODER 定角色，OTYPER 定驱动，OSPEEDR 定边沿，PUPDR 定默认电平，AFR 接外设。**

**手册依据**：RM0090 GPIO；AN4899。[^rm0090][^an4899]

## Q71：推挽和开漏怎么选？GPIO speed 是输出频率吗？

**回答**：推挽能主动拉高和拉低，适合普通数字输出；开漏只能主动拉低，高电平依赖上拉，适合 I²C、线与、多电压域兼容场景。`OSPEEDR` 调节输出驱动边沿/带宽能力，不是自动输出某个时钟频率。速度档过高会增加 EMI、串扰和动态功耗，应选满足信号时序的最低档。

**记忆**：**推挽上下都能推，开漏只会拉低；speed 管边沿，不是波形频率。**

**手册依据**：RM0090 GPIO output configuration；AN4899 electrical/low-power guidance。[^rm0090][^an4899]

## Q72：定时器更新频率如何计算？

**回答**：对普通向上计数且每次溢出更新的典型配置：

$$f_{update}=\frac{f_{TIM}}{(PSC+1)(ARR+1)}$$

因为 PSC 和 ARR 寄存器表示的都是“计数上限减一”。中心对齐模式、重复计数器、外部时钟、级联或更新源设置会改变事件频率，必须按对应模式重新计算。PSC/ARR 具有 preload/shadow 行为时，新值可能到 update event 才生效。

**记忆**：**先算真实 TIM 时钟，再除 `(PSC+1)(ARR+1)`；模式变化要重算。**

**手册依据**：RM0090 Timer chapters；AN4013；AN4776。[^rm0090][^an4013][^an4776]

## Q73：输入捕获、输出比较和 PWM 的本质区别是什么？

**回答**：输入捕获在指定输入边沿把 `CNT` 锁存进 `CCR`，用于测周期/脉宽；输出比较在 `CNT==CCR` 时触发输出动作、事件、中断或 DMA；PWM 是输出比较的一类周期波形模式，由 ARR 定周期、CCR 定有效时间，极性和计数模式决定最终占空关系。高级定时器还涉及互补输出、dead-time、break 和 repetition counter。

**记忆**：**捕获是“边沿到时抄 CNT”，比较是“CNT 到点做动作”，PWM 是周期性比较。**

**手册依据**：AN4013 Input capture/output compare/PWM；RM0090 TIM。[^an4013][^rm0090]

## Q74：ADC 位数等于实际精度吗？采样时间为何受源阻抗影响？

**回答**：N 位只定义量化码宽，实际准确度还受 offset、gain、DNL、INL、参考源、噪声、PCB、温度影响。SAR ADC 采样阶段要通过输入路径给内部采样电容充电；源阻抗越高，达到目标误差所需 acquisition time 越长。STM32F407 用 `ADC_SMPR1/2` 选择通道采样周期，转换时间还要加 SAR 转换周期。

**工程措施**：低噪声参考与去耦、合适驱动阻抗/运放、足够采样时间、校准/平均、避免数字开关噪声在采样点发生。

**记忆**：**分辨率说有多少格，准确度说每格靠不靠谱；高阻信号要给采样电容更多充电时间。**

**手册依据**：AN2834；RM0090 ADC；DS8626 ADC characteristics。[^an2834][^rm0090][^ds8626]

## Q75：UART 波特率误差、ORE、IDLE 分别是什么工程问题？

**回答（STM32F407）**：波特率由外设时钟、过采样配置和 `USART_BRR` 共同决定，发送端与接收端总误差必须落在数据帧可容忍范围内；具体容差查 datasheet。ORE 表示接收数据未及时读走又来了新数据，常见于中断延迟或 DMA/读取流程错误。IDLE 表示 RX 线在一帧后保持空闲，可用于不定长 DMA 接收的帧间判定，但标志清除顺序必须按 RM0090。

**记忆**：**BRR 管速率，ORE 说明来不及收，IDLE 可帮助切不定长帧。**

**手册依据**：RM0090 USART；DS8626 USART timing characteristics。[^rm0090][^ds8626]

## Q76：SPI 的 CPOL、CPHA、NSS 和全双工该怎样理解？

**回答（STM32F407）**：CPOL 定义空闲时 SCK 电平，CPHA 定义在第一个还是第二个时钟边沿采样；主从两端必须一致。NSS 可由硬件或软件管理，用来界定从设备选择和帧边界。标准全双工 SPI 每产生一个时钟，发送移位寄存器和接收移位寄存器同时移动，因此“只发送”也会产生接收数据；不及时处理可能 OVR。

**记忆**：**CPOL 看空闲电平，CPHA 看采样边沿；SPI 每打一个时钟都同时收发一位。**

**手册依据**：RM0090 SPI，`SPI_CR1/CR2/SR`。[^rm0090]

## Q77：为什么 I²C 必须开漏上拉？仲裁和 clock stretching 怎么工作？

**回答**：I²C 的 SDA/SCL 使用有线与结构：设备只主动拉低，释放后由上拉变高，允许多个设备共享且不发生高低电平硬对抗。多主仲裁时，主机发送“1”（释放 SDA）却读到“0”即丢失仲裁，并停止驱动；clock stretching 是从设备保持 SCL 低，延长低电平期让主机等待。上拉阻值需同时满足上升时间和灌电流限制。

**记忆**：**总线低电平是任何设备都能拉，高电平必须大家都释放；因此能无损仲裁。**

**手册依据**：NXP UM10204 I²C-bus specification；RM0090 I2C。[^i2c][^rm0090]

## Q78：CAN 为什么能做到非破坏性仲裁？

**回答**：CAN 总线有 dominant/recessive 位，多个节点同时发送时逐位监视总线；发送 recessive 却读到 dominant 的节点失去仲裁并停止发送，获胜帧不被破坏。标识符参与仲裁，因此数值更小的有效仲裁字段通常具有更高总线优先级。STM32F407 的 bxCAN 只实现控制器逻辑，物理总线还需要 CAN transceiver 和正确终端。

**记忆**：**显性位压过隐性位，发送者边发边听；输掉的安静退出，赢家报文继续。**

**手册依据**：Bosch CAN specification/protocol资料；RM0090 bxCAN。[^bosch-can][^rm0090]

## Q79：轮询、中断和 DMA 应怎样选？

**回答**：轮询简单、状态明确，适合启动早期、低速或严格短事务；中断适合稀疏异步事件，但每次事件有上下文开销；DMA 适合连续/批量传输，但增加缓冲区、总线竞争和一致性复杂度。工程选择看数据率、最大可容忍延迟、CPU 预算、突发长度和错误恢复，不是“DMA 永远高级”。

**记忆**：**少而短用轮询，稀疏异步用中断，连续批量用 DMA。**

**手册依据**：AN4031 DMA；RM0090 各外设的 polling/interrupt/DMA request 机制。[^an4031][^rm0090]

---

# 9. Cortex-M 系列差异：只记会改变工程结论的部分

| 能力 | M0/M0+ | M3 | M4 | M7 | M23/M33 | M55/M85 |
|---|---|---|---|---|---|---|
| 架构主线 | Armv6-M | Armv7-M | Armv7E-M | Armv7E-M | Armv8-M | Armv8.1-M |
| BASEPRI/FAULTMASK | 无 | 有 | 有 | 有 | 依架构/实现 | 有相应机制 |
| DSP 扩展 | 无专用 DSP 扩展 | 无专用 DSP 扩展 | 主要特征 | 有 | M33 可选 DSP | MVE/Helium 取决于型号 |
| FPU | 通常无 | 无 | 可选单精度 | 可选单/双精度实现相关 | M33 可选 | 型号/实现相关 |
| MPU | 可选/型号相关 | 可选实现 | 可选实现 | 可选实现 | 新版 MPU | Armv8.1-M MPU |
| TrustZone | 无 | 无 | 无 | 无 | M23/M33 支持安全扩展 | 支持安全扩展 |
| L1 Cache | 通常无 | 无 | 无内核 L1 | 可带 I/D Cache | 实现相关 | 型号相关 |

> 表中“可选/实现相关”必须落实到目标芯片 datasheet。Cortex-M 型号名不能替代芯片手册。

**手册依据**：CMSIS-Core Processor Support 列出的各 Generic User Guide；Arm 各内核 TRM。[^cmsis]

---

# 10. 面试速记：一个 MCU 的完整心智模型

面对任何 MCU 问题，按下面十层定位：

1. **执行层**：寄存器、Thumb 指令、特权级、MSP/PSP、ABI。
2. **异常层**：向量表、NVIC、优先级、屏蔽、自动压栈、Fault。
3. **地址层**：Flash/SRAM/外设/SCS 的映射、属性、MPU。
4. **启动层**：BOOT 映射 → MSP/Reset vector → startup → runtime → main。
5. **时钟电源层**：振荡器 → PLL → AHB/APB → 外设；电压/Flash latency/低功耗。
6. **并发层**：主循环、ISR、任务、DMA；volatile、临界区、原子与屏障各司其职。
7. **数据搬运层**：轮询/中断/DMA；缓冲区所有权、总线可达和 Cache 一致性。
8. **外设层**：先明确电气和时序，再看寄存器状态机，最后选 HAL/LL/裸寄存器。
9. **可靠性层**：Fault 证据、复位原因、watchdog、欠压、安全状态。
10. **产品升级层**：分区、镜像认证、断电恢复、试运行确认、回滚、防降级。

**最终记忆段落**：

> MCU 不是“一颗会跑 C 的芯片”，而是一套由处理器执行模型、异常系统、统一地址空间、片上总线、时钟电源、存储与外设共同组成的状态机系统。排查问题时，先定位它属于内核还是芯片，再沿“时钟是否到达—地址是否正确—状态/标志是否满足—中断/DMA 是否真的连通—内存可见性是否成立”逐层验证。Bootloader 和可靠性设计则必须始终考虑异常、掉电和版本不一致，不能只验证正常路径。

---

# 11. 官方手册索引

[^armv7m]: Arm, [ARMv7-M Architecture Reference Manual, DDI 0403](https://support.arm.com/documentation/ddi0403/ee/). 重点：Programmer's model、Memory model、Exception model、System address map、指令语义。

[^cm4dug]: Arm, [Cortex-M4 Devices Generic User Guide, DUI 0553](https://support.arm.com/documentation/dui0553/b/). 重点：Cortex-M4 programmer's model、exception model、core peripherals。

[^cm4trm]: Arm, [Cortex-M4 Technical Reference Manual, DDI 0439](https://developer.arm.com/documentation/ddi0439/latest/). 重点：处理器实现、NVIC、FPU、Debug、ACTLR。

[^cmsis]: Arm, [CMSIS-Core (Cortex-M) Overview](https://arm-software.github.io/CMSIS_6/latest/Core/index.html). 重点：core register abstraction、startup、system initialization、各 Cortex-M 文档入口。

[^cmsis-reg]: Arm, [CMSIS-Core Register Access](https://arm-software.github.io/CMSIS_6/latest/Core/group__Core__Register__gr.html). 重点：CONTROL、MSP/PSP、PRIMASK、BASEPRI、FAULTMASK。

[^cmsis-nvic]: Arm, [CMSIS-Core NVIC Functions](https://arm-software.github.io/CMSIS_6/v6.0.0/Core/group__NVIC__gr.html). 重点：priority grouping、encode/decode、enable/pending/active。

[^cmsis-intrin]: Arm, [CMSIS CPU Instruction Intrinsics](https://arm-software.github.io/CMSIS_6/v6.0.0/Core/group__intrinsic__CPU__gr.html). 重点：WFI/WFE/SEV、DMB/DSB/ISB、LDREX/STREX。

[^cmsis-cache]: Arm, [CMSIS Cortex-M7 D-Cache Functions](https://arm-software.github.io/CMSIS_6/latest/Core/group__Dcache__functions__m7.html). 重点：clean、invalidate、32-byte address alignment requirement。

[^cmsis-startup]: Arm, [CMSIS Startup File](https://arm-software.github.io/CMSIS_6/latest/Core/startup_c_pg.html). 重点：vector table、MSP、Reset_Handler、SystemInit 和 C/C++ runtime 交接。

[^cmsis-systick]: Arm, [CMSIS SysTick Configuration](https://arm-software.github.io/CMSIS_6/latest/Core/group__SysTick__gr.html). 重点：SysTick_Config、时钟源和异常。

[^aapcs]: Arm, [Procedure Call Standard for the Arm Architecture (AAPCS32)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst). 重点：core register usage、stack constraints、public interface alignment。

[^armlink]: Arm, [Arm Compiler for Embedded User Guide](https://developer.arm.com/documentation/100748/latest/). 重点：load region、execution region、scatter-loading、section placement。

[^armclang]: Arm, [Arm Compiler for Embedded Reference Guide - Qualifiers](https://developer.arm.com/documentation/101754/latest/Appendixes/Standard-C-Implementation-Definition/Qualifiers). 结合 ISO C volatile 规则使用；`volatile` 不等于原子或 Cache 一致性。

[^pm0214]: STMicroelectronics, [PM0214: STM32 Cortex-M4 MCUs and MPUs programming manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf). 重点：programmer's model、exceptions、NVIC、SCB、MPU、FPU、debug。

[^pm0253]: STMicroelectronics, [PM0253: STM32F7/H7 Cortex-M7 programming manual](https://www.st.com/resource/en/programming_manual/dm00237416-stm32f7-series-and-stm32h7-series-cortexm7-processor-programming-manual-stmicroelectronics.pdf). 重点：M7 memory model、MPU、L1 Cache。

[^rm0090]: STMicroelectronics, [RM0090: STM32F405/407/415/417/42x/43x Reference Manual](https://www2.st.com/resource/en/reference_manual/dm00031020.pdf). 重点：memory map、RCC、PWR、Flash、GPIO、EXTI、DMA、Timer、ADC、USART、SPI、I²C、bxCAN、IWDG/WWDG。

[^ds8626]: STMicroelectronics, [DS8626: STM32F405xx/STM32F407xx Datasheet](https://www.st.com/resource/en/datasheet/dm00037051.pdf). 重点：最大频率、时钟/PLL/ADC/通信接口电气特性、供电和封装差异。

[^an2606]: STMicroelectronics, [AN2606: Introduction to system memory boot mode on STM32 MCUs](https://www.st.com/resource/en/application_note/an2606-introduction-to-system-memory-boot-mode-on-stm32-mcus-stmicroelectronics.pdf). 使用时必须查对应器件小节和文档修订版本。

[^an4657]: STMicroelectronics, [AN4657: STM32 in-application programming using the USART](https://www.st.com/resource/en/application_note/an4657-stm32-inapplication-programming-iap-using-the-usart-stmicroelectronics.pdf). 重点：IAP 组织、下载流程与示例。

[^an4031]: STMicroelectronics, [AN4031: Using the STM32F2/F4/F7 DMA controller](https://www.st.com/resource/en/application_note/an4031-using-the-stm32f2-stm32f4-and-stm32f7-series-dma-controller-stmicroelectronics.pdf). 重点：DMA 架构、stream/channel/FIFO/double-buffer、bus matrix。

[^an4838]: STMicroelectronics, [AN4838: Managing memory protection unit in STM32 MCUs](https://www.st.com/resource/en/application_note/dm00272912-managing-memory-protection-unit-in-stm32-mcus-stmicroelectronics.pdf). 重点：MPU 区域、权限、内存属性与用例。

[^an4839]: STMicroelectronics, [AN4839: Level 1 cache on STM32F7 and STM32H7](https://www.st.com/resource/en/application_note/dm00272913-level-1-cache-on-stm32f7-series-and-stm32h7-series-stmicroelectronics.pdf). 重点：Cache 行为、DMA 一致性和维护策略。

[^an4891]: STMicroelectronics, [AN4891: STM32H72x/H73x/H74x/H75x system architecture and performance](https://www.st.com/resource/en/application_note/an4891-stm32h72x-stm32h73x-and-singlecore-stm32h74x75x-system-architecture-and-performance-stmicroelectronics.pdf). 重点：M7、Cache、TCM、AXI/AHB 总线可达性。

[^an5156]: STMicroelectronics, [AN5156: Introduction to security for STM32 MCUs](https://www.st.com/resource/en/application_note/an5156-introduction-to-security-for-stm32-mcus-stmicroelectronics.pdf). 重点：secure boot、secure firmware update、root of trust、MPU 和产品安全机制。

[^an4899]: STMicroelectronics, [AN4899: Guidelines for GPIO hardware settings and low-power consumption](https://www.st.com/resource/en/application_note/dm00315319-stm32-gpio-configuration-for-hardware-settings-and-lowpower-consumption-stmicroelectronics.pdf).

[^an4013]: STMicroelectronics, [AN4013: Introduction to timers for STM32 MCUs](https://www.st.com/resource/en/application_note/an4013-introduction-to-timers-for-stm32-mcus-stmicroelectronics.pdf).

[^an4776]: STMicroelectronics, [AN4776: General-purpose timer cookbook](https://www.st.com/resource/en/application_note/an4776-generalpurpose-timer-cookbook-for-stm32-microcontrollers-stmicroelectronics.pdf).

[^an2834]: STMicroelectronics, [AN2834: How to optimize ADC accuracy in STM32 MCUs](https://www.st.com/resource/en/application_note/cd00211314-how-to-get-the-bestadc-accuracy-in-stm32-microcontrollers-stmicroelectronics.pdf).

[^i2c]: NXP Semiconductors, [UM10204: I²C-bus specification and user manual](https://www.nxp.com/docs/en/user-guide/UM10204.pdf). 重点：open-drain/wired-AND、arbitration、clock stretching、rise-time/pull-up constraints。

[^bosch-can]: Robert Bosch GmbH, [CAN Protocols and official specifications](https://www.bosch-semiconductors.com/products/ip-modules/can-protocols/). 重点：dominant/recessive、non-destructive arbitration、Classical CAN/CAN FD。

---

## 附录 A：来源 PDF 的 21 个问题覆盖检查

| 来源问题 | 对应回答 |
|---|---|
| NVIC 优先级分组 | Q10 |
| 中断嵌套/同抢占级 | Q11 |
| EXTI 中断与事件 | Q15 |
| ISR 不能做什么/为什么不能 delay | Q16、Q17 |
| volatile | Q18、Q53～Q56、Q60 |
| HSI/HSE/LSI/LSE | Q21 |
| PLL 倍频链路 | Q22 |
| AHB/APB 分频 | Q23、Q24 |
| Stop 模式与恢复 | Q26、Q29 |
| GPIO 时钟使能 | Q27 |
| BOOT0/BOOT1 | Q30 |
| 向量表前两个字 | Q31 |
| startup 文件与 `.data/.bss` | Q32～Q34 |
| 0x08000000/0x20000000 | Q35 |
| Scatter file/RAM 函数 | Q37～Q39 |
| Flash 分区 | Q42 |
| Bootloader 跳转与 C 骨架 | Q44～Q46 |
| IAP/写 Flash/断电 | Q47、Q48 |
| CRC/MD5/签名 | Q49 |
| 双 Bank/A-B/回滚 | Q50、Q51 |

## 附录 B：使用这份知识库时的查手册顺序

1. 先看芯片 **Datasheet**：电气范围、最大频率、封装和外设是否存在。
2. 再看芯片 **Reference Manual**：外设状态机、寄存器、时序和清标志规则。
3. 内核问题看 **Arm Architecture Reference Manual / Generic User Guide / TRM**。
4. 编程接口看 **CMSIS-Core**；编译链接问题看 **ABI/Compiler/Linker Guide**。
5. 复杂用例看厂商 **Application Note**，但最终寄存器合法性仍回到当前器件 RM/DS。

> **最后一句**：任何“默认值、最大频率、地址范围、可访问性、清标志顺序、低功耗保留状态”，都必须在目标芯片与目标修订版本的 RM/DS 中再次确认。
