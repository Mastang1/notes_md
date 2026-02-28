---

---
---
这份笔记整理了 **Trampoline OS** 的核心概念、开发流程、资源链接以及针对你 Cortex-M7 平台的适配要点。你可以直接复制到你的 Obsidian 或知识库中。

---

# 笔记：Trampoline OS (开源 AUTOSAR OS 实现)

## 1. 核心简介

**Trampoline** 是一个开源的实时操作系统 (RTOS)，严格遵循 **OSEK/VDX** 和 **AUTOSAR OS 4.x** 标准。

- **开发者**：法国南特中央理工学院 (Ecole Centrale de Nantes) 的 IRCCyN 实验室。
    
- **定位**：主要用于教学、研究以及非安全关键（Non-Safety-Critical）的工业应用。它是了解 AUTOSAR OS 内部机制的最佳“白盒”工具。
    
- **核心特点**：
    
    - **静态配置**：所有对象（Task, Resource, Alarm）在编译前通过 OIL 文件定义，运行时无动态内存分配（No `malloc`），符合车规安全理念。
        
    - **多架构支持**：原生支持 Cortex-M (M0/M3/M4/M7), AVR, RISC-V, POSIX (Linux/macOS 用于仿真)。
        

## 2. 核心资源链接 (Bookmarks)

|**资源名称**|**说明**|**链接地址**|
|---|---|---|
|**GitHub 仓库**|核心源码，包含内核和架构移植|[TrampolineRTOS/trampoline](https://github.com/TrampolineRTOS/trampoline)|
|**官方文档**|readthedocs 文档，包含安装和 OIL 语法|[Trampoline Documentation](https://www.google.com/search?q=https://trampoline.readthedocs.io/en/latest/)|
|**OIL 语言规范**|OSEK Implementation Language 标准|[OSEK VDX OIL Standard (PDF)](https://www.google.com/search?q=https://portal.osek-vdx.org/files/pdf/specs/oil25.pdf)|
|**Cortex-M 示例**|移植时的最佳参考代码 (STM32/Kinetis)|[GitHub: examples/cortex-m](https://www.google.com/search?q=https://github.com/TrampolineRTOS/trampoline/tree/master/examples/cortex-m)|
|**Goil 源码**|Trampoline 的配置编译器 (基于 Go 语言)|包含在主仓库的 `goil` 目录下|

## 3. 开发工作流 (The AUTOSAR Way)

Trampoline 的开发流程与 Vector DaVinci/EB Tresos 逻辑一致，只是工具变成了命令行。

1. **编写应用代码 (.c)**：使用标准 API (如 `ActivateTask`, `GetResource`)。
    
2. **编写配置文件 (.oil)**：描述系统架构 (CPU, Task 优先级, Stack 大小, ISR 绑定)。
    
3. **生成代码 (Goil)**：运行 `goil` 工具，解析 `.oil` 文件，生成 `tpl_app_config.c` (包含调度表、堆栈数组) 和 `Makefile`。
    
4. **编译链接**：调用 GCC (arm-none-eabi-gcc) 编译内核、生成的配置代码和用户代码。
    

### 典型 OIL 文件示例

代码段

```
OIL_VERSION = "2.5";

IMPLEMENTATION trampoline {
    /* 定义硬件平台相关的属性 */
    TASK {
        UINT32 STACKSIZE = 1024;
    } ;
    ISR {
        UINT32 STACKSIZE = 1024;
    } ;
};

CPU my_cpu {
    OS my_os {
        STATUS = EXTENDED;
        STARTUPHOOK = TRUE;
    } ;

    /* 定义一个任务 */
    TASK my_task {
        PRIORITY = 1;
        AUTOSTART = TRUE { APPMODE = std; };
        ACTIVATION = 1;
        SCHEDULE = FULL;
    } ;
};
```

## 4. 针对 Cortex-M7 (S32G/K3) 的适配笔记

由于 Trampoline 官方仓库已有 **Cortex-M4 (ARMv7E-M)** 支持，适配 M7 的工作量极小。

### A. 目录结构对应

- 内核路径：`trampoline/os` (核心调度逻辑，无需修改)
    
- 移植路径：`trampoline/machines/cortex-m` (包含 SysTick, Context Switch)
    
    - 该目录下的汇编代码 (`tpl_ctx_switch.S`) 兼容 Cortex-M7。
        

### B. 关键修改点

1. **中断向量表 (Startup Code)**：
    
    - Trampoline 依赖 `PendSV` 进行上下文切换，依赖 `SysTick` 进行时间片轮转。
        
    - **操作**：确保你的启动文件 (`startup_S32G.s`) 中，中断向量名与 Trampoline 定义的一致。通常是将 `PendSV_Handler` 映射到 `tpl_primary_irq_handler` (或类似入口)。
        
2. **MPU & Cache (M7 特有坑)**：
    
    - Cortex-M7 带有 D-Cache。如果 Trampoline 的 Task Stack 位于 Cacheable 区域，且 DMA (如 NXP IPCF) 也在操作内存，会导致数据不一致。
        
    - **操作**：在 `SystemInit` 中，建议通过 MPU 将 RAM 设置为 Write-Through 或直接禁用 D-Cache 进行初期调试。
        
3. **FPU (浮点单元)**：
    
    - 如果你的任务涉及浮点运算，上下文切换必须保存 FPU 寄存器 (`s0-s31`)。
        
    - **操作**：检查 `.oil` 文件中是否有 `FPU = TRUE` 选项，并验证 `tpl_ctx_switch.S` 中是否有 `vstmdbeq` / `vldmiaeq` 指令。
        

## 5. 快速上手命令 (Cheat Sheet)

假设你已安装 Python 和 Go 环境：

Bash

```
# 1. 克隆仓库
git clone https://github.com/TrampolineRTOS/trampoline.git
cd trampoline

# 2. 编译 Goil 配置工具 (核心步骤)
cd goil
./makefile-unix.sh  # 或者 makefile-windows.bat
# 生成的可执行文件在 goil/goil

# 3. 编译一个 Cortex-M 示例 (以 STM32 为例，逻辑通用)
cd ../examples/cortex-m/armv7em/stm32f407/blink
# 运行 goil 生成代码
# -t: target (cortex-m/armv7em/stm32f407) 
# -g: 生成器模式
../../../goil/goil --target=cortex-m/armv7em/stm32f407 --templates=../../../goil/templates/ blink.oil

# 4. 此时目录下会生成 build.py 或 Makefile
# 修改编译器路径 (arm-none-eabi-gcc) 后运行编译
./build.py
```

## 6. 避坑指南

- **不要手写 Makefile**：尽量利用 OIL 文件生成构建脚本，手动维护很容易漏掉 Trampoline 的某些宏定义。
    
- **ISR 定义**：在 Trampoline 中，ISR 必须在 OIL 中定义为 `ISR category_2` (由 OS 接管) 或 `category_1` (绕过 OS)。不要直接在 C 代码里写 `void My_IRQHandler()`，需使用 `ISR(My_ISR_Name)` 宏。
    
- **Resource 限制**：如果你在 FreeRTOS 习惯了 Mutex，注意 AUTOSAR 的 Resource 必须严格遵循 LIFO (后进先出) 顺序释放，否则 OS 会报错 `E_OS_ACCESS`。