# Cortex-M7 Debug：以故障定位为主线的 9 个实战 Demo

> 适用：Armv7-M / Cortex-M7；优先使用 **已有的 Keil MDK + J-Link + CMSIS 工程**。文内按工程排障的**建议学习顺序**排列，不声称有行业使用频率统计。`debug_lab.c` 提供教程 Demo 1～7 的核心代码及独立的 Mode 8（CPU 寄存器/单步补充实验）；Demo 8 ETM 采用厂商现成工程，Demo 9 使用调试器手工读取寄存器。所有上板结果必须由实际芯片、下载算法、调试器版本和布线决定，**本包不声称已经在真实 MCU 上跑通**。
>
> **与上一轮回答的重要更正**：Armv7-M 架构 C1.11 描述 FPB 可做 Flash patch，但 **Cortex-M7 TRM 9.3 明确：Cortex-M7 FPB 只支持硬件断点，不支持 Flash patch，FP_REMAP 为 RAZ/WI**。ETM 完整指令轨迹也不能仅靠普通 SWO Viewer 获得；须核对芯片的 ETM、Trace 输出链路和调试探头。本教程以具体 M7 实现手册覆盖架构手册的通用功能描述。

<a id="setup"></a>

## 0. 开始前：一次性准备（10 分钟）

1. 在正常能下载的 **Cortex-M7 CMSIS 项目**中添加 [`debug_lab.c`](./debug_lab.c)。设备包/启动汇编、链接脚本、时钟初始化继续使用你原工程的；不要新建“脱离具体芯片的通用可下载工程”。
2. 工程必须能包含 `core_cm7.h` 并提供 `SystemCoreClock`；在 `main()` 中完成原有硬件/时钟初始化及 `SystemCoreClockUpdate()` 后调用 `DebugLab_Run();`。声明 `void DebugLab_Run(void);`。选不同源码 Demo 时在 C/C++ 编译宏中设置 `DEBUG_LAB_DEMO=1`～`7`；`DEBUG_LAB_DEMO=8` 是 Demo 4 附加的核心寄存器/单步练习。教程 Demo 8（ETM）与 Demo 9（DAP/ID）不使用此编译宏。**每次只运行一个源码模式**。
3. Keil：Options for Target → Debug 选择 J-Link；选择**真实芯片型号**、SWD、正确 Flash Download 算法；设置编译产生调试信息。先用 `-O0` 调故障和断点；CRC 性能实验改为 `-O2` 且保留相同构建配置做比较。软件断点/硬件断点数量受工具和芯片配置限制。
4. `debug_lab.c` 的 Demo 2 自定义 `HardFault_Handler`，需确认原启动文件只有 **weak** 同名处理函数；若工程已实现自己的 HardFault handler，**替换/整合，不要并存两个强定义**。所有故意触发故障和软件复位的 Demo 仅用于隔离测试板，禁止直接并入产品。
5. 先记录硬件能力：Keil Memory 窗口读 `0xE000ED00`（CPUID），`0xE0001000`（DWT_CTRL），`0xE0002000`（FP_CTRL）。如果地址访问失败、调试保护已打开或器件未实现相关 IP，停止对应实验，不要向猜测的 MMIO 地址写值。Cortex-M7 的部分 Trace、ETM、DWT 数目由芯片集成决定。[Arm M7 TRM §1.1.3/§9/§11](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)；[CMSIS Cortex-M7 寄存器定义](https://github.com/ARM-software/CMSIS_5/blob/develop/CMSIS/Core/Include/core_cm7.h)。

**低成本必备硬件**：目标板、电源、J-Link、SWDIO/SWCLK/GND（建议连接 nRESET）。**可选**：SWO 实验需要目标 SWO 引脚和探头 SWO 支持；ETM 需要支持 ETM 的 MCU、实际引出的 Trace 引脚或片上 Trace buffer，以及相应采集探头。SWD 两根信号线不等于具有 SWO/ETM 能力。[SEGGER SWO 使用说明](https://kb.segger.com/SWO)。

## 路线图：优先解决真正会遇到的 Bug

| 顺序 / Demo | 你面对的故障 | 首先使用的模块 | 上板验收 |
|---|---|---|---|
| [1](#demo-1-变量被谁写坏了dwt-watchpoint) | 接收队列索引偶发变坏 | C1.8 DWT | 停在真正写坏数据的指令 |
| [2](#demo-2-hardfault-第一现场与调用栈) | MCU 崩溃、落入 HardFault | C1.4/C1.5/C1.6 | CFSR 与 HFSR 能解释故障 |
| [3](#demo-3-30-kb-crc-真实耗时) | 通信线程 CRC 占用未知 | C1.8 DWT CYCCNT | 5 次原始周期数 + CRC 正确性 |
| [4](#demo-4-flash-代码的硬件断点与单步) | Flash 代码里需要准确停住 | C1.11 FPB + C1.6 | 停在 TxSend，单步核实数据 |
| [5](#demo-5-复位后启动代码立即跑飞) | SystemInit/Bootloader 上电跑飞 | C1.4 Reset Catch | 停在 Reset_Handler 第一指令附近 |
| [6](#demo-6-不停机记录-txrx-事件) | 断点会改变 RTOS 收发时序 | C1.7 ITM + C1.10 TPIU | SWV 看见 T/R/E 标记 |
| [7](#demo-7-cpu-为何突然暂停) | 异常/断点原因不明 | C1.5 DFSR | halt 时读取并解释 DFSR |
| [8](#demo-8-偶发跑飞的指令历史etm-条件实验) | 看崩溃之前的历史指令 | C1.9 ETM + C1.10 | Trace 窗口还原执行路径 |
| [9](#demo-9-调试器连不上或组件能力不匹配) | 连不上、找不到 DWT/ITM | C1.1/C1.2/C1.3 + 附录 | ROM Table / ID / 芯片能力吻合 |

<a id="demo-1"></a>

## Demo 1：变量被谁写坏了？DWT Watchpoint

**场景**：`rx_index` 理应为接收缓冲区合法索引，但被另一条路径错误覆盖。常规 Watch 窗口只能看到值；**数据写入断点**才能抓到写入现场。

1. `DEBUG_LAB_DEMO=1`，下载，先在 `g_lab_stage = 1u` 后面 `__NOP();` 这行设置**普通代码断点**，运行并停住。此时 `g_rx_index = 0` 已初始化。
2. Keil → Debug → Breakpoints（Ctrl+B），Expression 输入 `g_rx_index`，**Access 只勾选 Write**、宽度 4 Bytes、无数据值条件；Define。也可在 µVision 命令行按手册使用 `BA WRITE g_rx_index, 4`，不同 µVision 版本命令所需参数以本地 Help 为准。
3. Continue。观察是否停在 `DebugLab_BadWriter()` 中 `g_rx_index = 0xFFFFFFFFu;` 对应的 store 处/紧邻位置；在 Disassembly 里核实真实 `STR` 指令。记录 PC、调用栈、变量地址、DWT comparator。
4. 去掉该错误赋值或加入正常范围检查，再运行；不再命中错误写入，构成修复验收。实际项目把 watchpoint 设在 `rx_ring.read_idx`、DMA 描述符指针或队列写指针即可。

**寄存器机制**：`DWT_CTRL.NUMCOMP` 确认比较器数量；`DWT_COMPn` 放地址，`DWT_MASKn` 选范围，`DWT_FUNCTIONn` 选写入触发；Debug event 进入 halting debug。**让 IDE 管理比较器**，不要同时由固件写 DWT_FUNCTIONn，否则会覆盖 IDE 设置。watchpoint 是地址访问事件，不是 Watch 窗口每隔一段时间读一次变量。不同芯片具体可匹配的范围/数据值能力需要回读确认。[Armv7-M ARM §C1.8.1～C1.8.2、§C1.8 comparator registers](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[Keil 官方 Access Breakpoints 操作实例](https://www.keil.com/appnotes/files/apnt_236_v2.9.pdf)（Watchpoints 章节）；[Keil BA WRITE 命令](https://www.keil.com/support/man/docs/uv4cl/uv4cl_cm_breakaccess.htm)。

**失败分支**：一继续就停在初始化写入 → 在赋值完成后才建断点；找不到变量 → 开调试信息、`g_rx_index` 是 `volatile` 全局；`Cannot set breakpoint` → 检查 DWT 数量、对齐、其他 watchpoints 是否耗尽。DMA/其他总线主设备的写入**不一定经过 CPU DWT 比较路径**；CPU watchpoint 未命中不能证明没有 DMA 写坏内存，需要改查 DMA 描述符与总线/MPU 证据。

<a id="demo-2"></a>

## Demo 2：HardFault 第一现场与调用栈

**场景**：调试中的不合法指令触发 UsageFault；将 UsageFault 关闭，使它**升级为 HardFault**。这比向不确定 SoC 地址乱写更可复现。

1. `DEBUG_LAB_DEMO=2`；确认自定义 `HardFault_Handler` 覆盖的是 weak 默认处理函数。Keil Debug 设置启用 **HardFault vector catch**（选项名随工具版本变化）；或在已启用 halting debug 时由调试器设置 `DEMCR.VC_HARDERR`，**不要在运行的固件里随意改 DHCSR**。
2. 下载/Reset → Run。执行 `UDF #0` 后 CPU 应因 HardFault 在 handler 入口 halt。先看 `SCB->CFSR` 地址 `0xE000ED28`、`SCB->HFSR` 地址 `0xE000ED2C`，如停在 handler 第一条指令，可以单步到快照赋值后看 `g_fault_*`。
3. **预期**：新鲜复位且状态寄存器未被其他代码污染时，`CFSR.UFSR.UNDEFINSTR=1`（CFSR bit16），`HFSR.FORCED=1`（HFSR bit30）；软件修复前 `g_lab_stage` 不会到 2。检查 xPSR/PC、Disassembly 与调用栈定位 UDF。
4. 需要查异常前 PC：读异常进入时 LR 中的 `EXC_RETURN` 判断之前使用 PSP/MSP（bit2）；在**基础栈帧完整有效**时，核心帧 `R0,R1,R2,R3,R12,LR,PC,xPSR`，其中栈帧内 PC 偏移 24 字节。**M7 开启浮点扩展栈帧、懒保存、异常入栈失败或栈损坏时不可机械套用 `SP+24`**；用 IDE 的 exception-aware call stack 或按手册识别实际帧。不要把 handler 当前 PC 当成致错指令。
5. 在源码中移除 UDF，重新烧录并复位，故障消失。产品问题则按 `CFSR → HFSR → BFAR/MMFAR 是否有效 → 栈帧 → 反汇编` 处理；M7 的 `ABFSR` 对某些不精确 BusFault 可辅助判别哪条总线出错。

**现场判断**：`CFSR` 多个位可同时置位；`BFAR` 只有 `BFARVALID=1` 才可作定位依据，`MMFAR` 同理。`IMPRECISERR` 的 stacked PC **不保证就是出错 store**；M7 不应照搬 M3/M4 关闭写缓冲来“强制精确”的方法。Fault 状态位采用写 1 清除，先保存、后按手册清理，避免丢失故障证据。[Armv7-M ARM §B1.5 Fault behavior、§B3.2 CFSR/HFSR、§C1.4～C1.6](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[Arm 官方 Fault Handling 实战](https://developer.arm.com/community/arm-community-blogs/b/embedded-and-microcontrollers-blog/posts/debugging-embedded-systems-part-2-fault-handling-and-diagnosis)；[SEGGER 有现成故障工程和逐窗调试演示](https://kb.segger.com/Analyzing_Cortex-M_Faults_with_Ozone)；[Memfault 附栈帧/精确与不精确 BusFault 演示](https://interrupt.memfault.com/blog/cortex-m-hardfault-debug)。

<a id="demo-3"></a>

## Demo 3：30 KB CRC 真实耗时

**场景**：400 MHz Cortex-M7 的 RTOS TX 线程，对 30 KiB payload 执行逐位 CRC32；担心吞掉 RX 线程 CPU 时间。此例用你给出的**右移、反射多项式**写法，明确固定为 CRC-32/ISO-HDLC（poly `0xEDB88320`、init/xorout `0xFFFFFFFF`），不暗示与你工程私有宏的取值必然相同。

1. `DEBUG_LAB_DEMO=3`，代码占用额外 `30*1024 = 30720` 字节静态 RAM；先核对目标 RAM/链接映射，再烧录。设置 `-O2` 并保留调试信息。`SystemCoreClockUpdate()` 后调用本 Demo，**运行测量期间不要打断点**。
2. 下载运行结束后暂停，查看：`g_crc_known_value` 必须是 `0xCBF43926`（输入 ASCII `123456789` 的校验值）；`g_lab_result=0`、`g_lab_stage=2`。若 `g_lab_result=2` 则实现中无 CYCCNT，停止实验，改用板级定时器/逻辑分析仪。
3. 查看 `g_core_clock_hz` 及 `g_crc_cycles[0..4]`。计算每次 `ms = cycles*1000 / g_core_clock_hz`，并记录最大值、最小值、平均值。**不要把本教程的预估毫秒数当成实测值**。
4. 原样改成 `-O0` 再测；接着把数据放在实际 DMA RX buffer/不同 SRAM 区域对比。测试时记录 I/D-cache、TCM/AXI SRAM、总线争用、中断频率。自然运行的周期数包含被 ISR 打断的时间；高实时性 RTOS 上**不要为隔离 CRC 而关中断 10 ms**。
5. 若每隔 `P` ms 执行一次、一次用 `T` ms，则单核 CRC 负载约 `T/P`；例如某次实测得到 8 ms、周期 20 ms，CRC 约占该核 40%，还需另外计算线程切换/收发操作。**结论以你的样机测得的 `cycles` 为准。**

**寄存器与边界**：`DEMCR.TRCENA=1`，`DWT_CTRL.NOCYCCNT=0` 才允许启用 `CYCCNTENA`。`DWT_CYCCNT` 是 **32 位**计数，溢出按模 2³² 回绕；测量区间必须短于一次回绕并保证始终使用正确 CPU 时钟。DWT 在 Debug halt 时不计数；代码中的 `__DSB()/__ISB()` 是测量边界，不保证消除 cache / 中断干扰。`DWT_CPICNT/EXCCNT/LSUCNT` 等为**可选且多数为 8 位**，并非可以随意读取并作“整个 30 KB 全过程统计”；分段及溢出事件需要专门处理。[Armv7-M ARM §C1.8.3、§C1.8.7～C1.8.14](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[Cortex-M7 TRM §11](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)；[SEGGER 官方 CPU 性能测量实践](https://kb.segger.com/HowTo_Measure_CPU_Performance)。

**验收记录模板**：`chip/revision | compiler/version/-O2 | clock | code+data memory | cache state | IRQ state | 5×cycles | 5×ms | CRC known-value`。无此记录不能比较两种优化方案。

<a id="demo-4"></a>

## Demo 4：Flash 代码的硬件断点与单步

**场景**：通信函数不允许修改 Flash 内容，但需要捕获进入 `TxSend()` 时的入参。

1. `DEBUG_LAB_DEMO=4`，Keil 调试窗口在 `DebugLab_TxSend()` 内的 `g_tx_payload = value;` 设代码断点；下载后 Run。
2. 应停在该语句附近；看 `value=0x12345678`（表达式或寄存器呈现依优化而异），单步检查 `g_tx_payload`、`g_tx_count`。必要时切到 Disassembly，以实际 PC/写内存指令判定。
3. 查看 `FP_CTRL` 地址 `0xE0002000`，确认芯片提供的指令比较器数量；暂时删去其它断点，排除硬件槽位不足。
4. 注意 **M7 不支持 FPB Flash patch**：`FP_REMAP` 在 Cortex-M7 上未实现。这里是**比较指令获取地址并触发 BKPT**，不是将程序改映射到 RAM。SEGGER 的“Flash Breakpoints”软件扩展也不能被误称为“FPB 无限制硬件断点”。

**核心寄存器附加练习（`DEBUG_LAB_DEMO=8`）**：在 `__NOP()` 停下 → Keil Registers 窗口查看 R0～R12、MSP/PSP、LR、PC、xPSR → 单步到 `g_lab_register_demo ^= 0xFFFFFFFFu` → 验证变量由 `0x11223344` 变成 `0xEEDDCCBB`。再读 `0xE000EDF0` 的 `DHCSR.S_HALT`；DCRSR/DCRDR 用于调试器访问内核寄存器，**不要用固件和 IDE 同时争夺它们**。

**关键寄存器**：`FP_CTRL`（使能/数量）、`FP_COMPn`（指令地址比较器）；运行/单步控制关联 `DHCSR.C_DEBUGEN/C_HALT/C_STEP`，读取内核 R0/PC 等关联 `DCRSR/DCRDR`，这些由 debugger 调度，固件不应抢写。进 Debug state 后可读 `DHCSR.S_HALT`。**不要直接在运行状态改 `DHCSR.C_STEP/C_MASKINTS`**。[Cortex-M7 TRM §9.3：FPB 不支持 patch](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)；[Armv7-M ARM §C1.6、§C1.11](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[SEGGER 区分硬件/软件/Flash breakpoints](https://kb.segger.com/UM08001_J-Link_/_J-Trace_User_Guide)。

**失败分支**：断点灰色 → 核对 Flash download/符号地址、优化和比较器数量；源码行不对应单指令 → 看反汇编；停在 Flash 算法自身 → 先完成下载再安装断点；`-O2` 内联导致找不到函数 → 实验改 `-O0`，产品性能测量再恢复优化。

<a id="demo-5"></a>

## Demo 5：复位后启动代码立即跑飞

**场景**：上电 `SystemInit()` PLL/MPU 配置错误；普通 Attach 时程序已经跑过错误位置。

1. 选 `DEBUG_LAB_DEMO=5`；先通过 Debugger 的 **Reset and Halt / Reset Catch** 选项配置（如果只有 Reset 后自动 Continue 的选项，改为 Halt）。在硬件层有 nRESET 更便于可靠连接。
2. 在启动汇编 `Reset_Handler` 入口放断点，进行硬件 reset 或 debugger 的 reset-and-halt。应停在复位入口，之后单步经过数据/BSS 初始化、`SystemInit()`、`main()`。
3. 如果需要验证软件复位流程，取消入口断点 → 在 Demo 5 的 `NVIC_SystemReset()` 前停一下 → 再配置 Reset Catch 并执行。该函数会重复复位形成循环测试；结束实验必须改回其他 Demo。
4. 理解底层为 `DEMCR.VC_CORERESET` + halting debug（`DHCSR.C_DEBUGEN`），但**让调试器操作**：不同 SoC 的 POR、局部复位、看门狗复位可能影响 debug power domain 和 attach，不保证所有 reset 类型都可捕获。

**验收**：能在没有执行用户初始化代码前看到 Reset_Handler/PC；如果仍然跳到 main，检查调试器 reset strategy、芯片 boot ROM、nRESET 接线、连接时机。优先将 SEGGER Reset Type 保持默认自动选择，并**正确选择具体器件**。[Armv7-M ARM §C1.4 Reset Catch](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[SEGGER 官方 J-Link Reset Strategies](https://kb.segger.com/J-Link_reset_strategies)。

<a id="demo-6"></a>

## Demo 6：不停机记录 TX/RX 事件（ITM + TPIU + SWO）

**场景**：给 TX、RX 打断点导致高频通信失真，想用低成本 Trace 观察事件先后，而不是逐字节 `printf()`。

1. 必须确认芯片支持 ITM+TPIU、板子引出 **SWO**、探头连了 SWO；在 Keil Debug → Settings → Trace 配置**真实 Core Clock** 和支持的 SWO bitrate，开启 ITM stimulus port 0；打开 Debug (printf) Viewer / SWV。不同 Keil/J-Link 版本的界面名称可能不同，以当前 IDE 手册和探头支持情况为准。
2. `DEBUG_LAB_DEMO=6`，运行后看 port 0 输出的 `T`（TX Start）、`R`（RX Event）、`E`（TX End）。`g_lab_result` 表示**已被 ITM FIFO 接受**的字符个数，不等于 PC 真正收到多少个。
3. 将 `DebugLab_TryTraceChar('T')` 放到实际 TX 线程临界路径、`'R'` 放到 RX ISR、`'E'` 放到 TX 完成路径；必要时改用不同 stimulus ports 并配置 decoder，先量化记录丢失和采样扰动。
4. 断开 SWO 后再测：本示例为**非阻塞、允许丢事件**，无输出不应阻塞主线程；CMSIS 的 `ITM_SendChar()` 在已启用但输出口暂不可写时可能**阻塞等待**，严禁不加判断地用于高频 ISR。输出吞吐超过 SWO 时必须减少事件或使用更高带宽路径。

**底层**：`DEMCR.TRCENA → ITM_TCR.ITMENA → ITM_TER.bit0 → STIM[0] → TPIU → SWO`。`TPIU_SPPR` 选串行协议，`TPIU_ACPR` 为分频设置；不要在固件和 IDE 两处互相覆盖 Trace 配置。**ETM 指令 trace ≠ ITM 的 SWO 字符流**。[CMSIS 官方 ITM_SendChar 使用说明和阻塞语义](https://arm-software.github.io/CMSIS_5/latest/Core/html/group__ITM__Debug__gr.html)；[SEGGER SWO setup/limitations](https://kb.segger.com/SWO)；[Cortex-M7 TRM §12、§13](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)。

**验收/失败**：无字符时按 `pin mux/焊点 → probe SWO 支持 → Core Clock → bitrate → ITM/TER → Viewer` 顺序排查；SWO 电平波形正常但解码失败，优先核对 Trace 时钟和实际 SystemCoreClock；代码停顿明显，确认没有误用阻塞式 ITM/printf。

<a id="demo-7"></a>

## Demo 7：CPU 为何突然暂停？DFSR + Debug event

**场景**：程序在没有打源码断点的位置停止；区分主动 Halt、代码 BKPT、DWT 和 Fault vector catch。

1. `DEBUG_LAB_DEMO=7`，运行到 `__BKPT(0)`。**必须已连接 halting debugger**；未连接调试器执行 BKPT 可能进入 DebugMonitor 或引发 HardFault，不能把该 Demo 放产品固件中。
2. 当 CPU 停住，立刻在 Memory Window 读 `DFSR = *(uint32_t *)0xE000ED30`，读 `DHCSR = *(uint32_t *)0xE000EDF0`。`DFSR` 可含 `HALTED(bit0)、BKPT(bit1)、DWTTRAP(bit2)、VCATCH(bit3)、EXTERNAL(bit4)` 中一个或多个；该 Demo 通常涉及 BKPT，但 debugger 可能已读/清除 DFSR，**不能硬性要求读到固定数值**。
3. 将实验替换为 Demo 1 watchpoint，再看 DWTTRAP；替换为 Demo 2 开启 Fault Vector Catch 再看 VCATCH；在 IDE 点 Pause 则检查 HALTED。每次抓到后保存寄存器快照，再分析来源。
4. `DFSR` 是**写 1 清除**状态位；不应用 `SCB->DFSR = 0` 期望清除。查看状态时避免调试脚本先清空证据。

**寄存器**：`DHCSR` 控制/反映 Halt/Step；`DFSR` 记 debug 事件；`DEMCR` 管 vector catch 和 DebugMonitor。`DebugMonitor` 与传统 halt 模式是有条件互斥的事件处理路径，**不要简单认为 BKPT 永远会停在 IDE**。[Armv7-M ARM §C1.5～C1.6、DFSR](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[SEGGER Fault/Vector Catch 实操](https://kb.segger.com/Analyzing_Cortex-M_Faults_with_Ozone)。

<a id="demo-8"></a>

## Demo 8：偶发跑飞的指令历史（ETM，条件实验）

**场景**：数小时后碰到间接跳转错误，HardFault 只能证明最后状态；需要知道出错前经历哪些函数/指令。

**这里采用真正提供配套工程的厂商指导，不编造“通用 M7 的 ETM 启动代码”。** 首选与 Cortex-M7 匹配的 [SEGGER NXP S32K3xx Trace 官方实测工程与具体探头要求](https://kb.segger.com/NXP_S32K3xx)；若你有 SEGGER Cortex-M Trace Reference Board，可另看 [Ozone Trace Tutorial（该示例板为 M4，流程可借鉴，但不是 M7 实测证明）](https://www.segger.com/products/development-tools/ozone-j-link-debugger/technology/ozone-trace-tutorial/)。

1. 用器件 TRM/SoC RM 确认集成了 ETM、存在所需 Trace 端口或片上 Trace buffer；确认探头是 **J-Trace PRO 或文档声明支持该具体芯片/采集方式的型号**，不要默认“普通 J-Link + SWO = ETM”。
2. 从 S32K3 官方 SEGGER 页面选对应 **Cortex-M7 核**及 streaming/trace-buffer 示例并下载其项目；按页面要求连好目标板/探头、指定完整设备型号，打开厂商提供的 `.jdebug` 项目。
3. 打开 Instruction Trace 和 Code Profile → 下载/复位 → Run → 复现异常 → 在 Trace 窗口逆向查看函数/分支历史，与反汇编、ELF 对照；看到“故障前轨迹”才算成功。如果设备/探头不匹配，只能登记 **Not supported/not tested**，不能伪报通过。
4. 检查 Trace data overflow、trace clock、bandwidth、芯片 pin mux、trace-buffer 容量。**M7 ETM-M7 用 ETMv4 模型**；当 TPIU 处于 SWO formatter bypass 模式时 ETM 数据会被丢弃，SWO printf Viewer 不是指令历史窗口。[Cortex-M7 TRM §13.2.3](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)；[Arm ETM-M7 TRM §1～§3](https://documentation-service.arm.com/static/5e906dfe8259fe2368e2ac54)。

**寄存器**：不要拿 ARMv7-M C1.9 的通用旧 ETM 描述编造 M7 寄存器写法；按 ETM-M7 TRM 的 `TRC*` 配置和设备 trace topology 管理。结果必须保存 `chip + probe + trace source + trace clock + loss flags + .jdebug` 及实际 trace 截图/导出。

<a id="demo-9"></a>

## Demo 9：调试器连不上或组件能力不匹配（DAP + CoreSight ID）

**场景**：同是 Cortex-M7，A 板有 Trace，B 板没 Trace；或 J-Link 只读到总线而 IDE 误认设备。

1. 先排外部链路：板上电压/公共地、SWDIO/SWCLK/nRESET、Boot strap、芯片是否进入安全锁定、J-Link 已选正确 device；降低 SWD 频率并通过标准连接操作读取 CPUID `0xE000ED00`。不要把 `0xFFFFFFFF` 或 AP fault 解释为“功能位全为 1”。
2. 使用 IDE Memory（**32-bit read**）读取 M7 **Processor ROM Table** `0xE00FE000`：`[0]=0x00001003` 指向 PPB ROM Table；`[1]=0xFFF43003` 表示 ETM 存在（`...002` 为不存在）；再读 **PPB ROM Table** `0xE00FF000`：`[0] SCS、[1] DWT、[2] FPB、[3] ITM`。**这组具体值仅作为 Cortex-M7 TRM 参考实现，SoC 可能还有上一级 system ROM table；Debugger 应从 DP/AP 枚举而不是假定第一个 ROM Table 就是这一张。**
3. 可读 M7 Processor ROM Table 的 CID0～3：`0xE00FEFF0=0x0D`、`0xE00FEFF4=0x10`、`0xE00FEFF8=0x05`、`0xE00FEFFC=0xB1`；PPB ROM Table 对应地址 `0xE00FFFF0`～`0xE00FFFFC`。按 `PID0～PID4` 判断 part/designer/revision，**CID/PID 是标识，并非启用 ITM 的控制位**。
4. 对已确认存在的 DWT 再读 `DWT_CTRL.NUMCOMP、NOCYCCNT、NOPRFCNT、NOTRCPKT`；FPB 读 FP_CTRL 获取实际指令比较器数量。ITM、ETM、TPIU 还必须由 ROM Table、芯片手册、外部引脚/接口共同确认。
5. 若 J-Link 连接成功但 Memory 访问受拒，可能是 AP 选择、安全策略或芯片 debug power domain；不能仅凭固定 `0xE000...` 地址就断言器件功能。试官方 probe 的 device-specific Connect/Reset 流程，不要自行写不明用途的 unlock key。

**机制**：`SWD/JTAG → DP → AP/MEM-AP → system bus → CoreSight ROM Table → component ID → SCS/DWT/FPB/ITM`。这是工具自动发现调试拓扑的过程，不是代码运行时检测所有 IP 的万能接口。D1 附录（截图旧版叫 A.1）给出 `PIDx/CIDx` 和 `LAR/LSR` 等管理寄存器；`LAR/LSR` 只有组件实际实现软件锁时才有意义。[Cortex-M7 TRM §9.1.1～§9.1.4，包含准确 ROM Table 值](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)；[Armv7-M ARM §C1.2、§C1.3、Appendix D1](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)；[SEGGER 正确选择芯片和 reset strategy](https://kb.segger.com/J-Link_reset_strategies)。

<a id="topic-coverage"></a>

## 11 个 Topic / 附录覆盖核对（不按目录顺序讲课，但一项不遗漏）

| 用户截图中的原始标题 | 教程对应 |
|---|---|
| ARMv7-M Debug（总章） | §0、路线图、Demo 9 的总体数据路径 |
| C1.1 Introduction to ARMv7-M debug | §0、Demo 7 的 halt vs DebugMonitor、Demo 9 |
| C1.2 The Debug Access Port | Demo 9 的 SWD → DP → AP → Memory |
| C1.3 Overview of ARMv7-M debug features | §0 的能力检查、Demo 9 的 discovery |
| C1.4 Debug and reset | Demo 5，Demo 2 的 fault catch |
| C1.5 Debug event behavior | Demo 7，Demo 1/2 的 event 来源 |
| C1.6 Debug register support in the SCS | Demo 2/4/5/7（DHCSR/DCRSR/DCRDR/DEMCR/DFSR） |
| C1.7 The Instrumentation Trace Macrocell | Demo 6 |
| C1.8 The Data Watchpoint and Trace unit | Demo 1、Demo 3 |
| C1.9 Embedded Trace Macrocell support | Demo 8，采用 M7 专用 ETM-M7 文档 |
| C1.10 Trace Port Interface Unit | Demo 6（SWO）、Demo 8（ETM 数据路径） |
| C1.11 Flash Patch and Breakpoint unit | Demo 4；特别纠正 M7 不支持 patch |
| Appendices（目录分隔标题） | Demo 9 的附录/识别寄存器说明 |
| ARMv7-M CoreSight Infrastructure IDs | Demo 9 |
| A.1 CoreSight infrastructure IDs for an ARMv7-M implementation | Demo 9；新版对应 D1.1 |

**补充 review 警戒线**：C1.11 属架构通用规格，而 M7 TRM §9.3 限定实际 FPB；C1.9 对 ETM 的通用描述不能直接代替 M7 的 ETMv4；TPIU 和实际引脚由芯片集成商决定；DWT 监测 CPU 数据访问不能直接证明 DMA 外设未写 RAM；Debug breakpoint/ITM 都可能扰动实时性；CRC 性能比较必须保存相同编译器/优化/缓存/存储位置。以上任一条件不成立，就将对应实验判为条件未满足，而不是编造通过结果。

<a id="references"></a>

## References / Manuals（按查用顺序）

- **Armv7-M Architecture Reference Manual，DDI 0403E.e**：[官方原文](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f)，重点 C1.1～C1.11、Appendix D1（旧版 DDI 0403D 与截图同名附录叫 A.1）；另看 B1.5 fault、B3.2 SCS。**手册版本不同，印刷页码变化，优先使用小节号。**
- **Cortex-M7 Processor TRM，DDI 0489F**：[官方原文](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe)，§1.1.3 可选能力、§9 FPB/ROM、§11 DWT、§12 ITM、§13 TPIU。
- **CoreSight ETM-M7 TRM，DDI 0494D**：[官方原文](https://documentation-service.arm.com/static/5e906dfe8259fe2368e2ac54)，§1/2/3。
- **CMSIS Core Cortex-M7 代码**：[core_cm7.h](https://github.com/ARM-software/CMSIS_5/blob/develop/CMSIS/Core/Include/core_cm7.h)；[ITM API](https://arm-software.github.io/CMSIS_5/latest/Core/html/group__ITM__Debug__gr.html)。以用户设备包实际版本为准，不要从 GitHub 盲拷贝头文件替换 SDK。
- **实际调试方法**：[Arm Fault Diagnosis](https://developer.arm.com/community/arm-community-blogs/b/embedded-and-microcontrollers-blog/posts/debugging-embedded-systems-part-2-fault-handling-and-diagnosis)、[Keil Watchpoints 应用笔记](https://www.keil.com/appnotes/files/apnt_236_v2.9.pdf)、[SEGGER Ozone Fault](https://kb.segger.com/Analyzing_Cortex-M_Faults_with_Ozone)、[SEGGER SWO](https://kb.segger.com/SWO)、[SEGGER S32K3 ETM 工程](https://kb.segger.com/NXP_S32K3xx)。

**复现记录**：对每个 Demo 使用 [`review_report.md`](./review_report.md) 的验收矩阵；“手册核对”“语法检查”“目标板已运行”必须分开填写，缺乏板卡日志时不能填硬件通过。
