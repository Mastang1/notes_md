
## 1. BSP 的三层核心模型

所以可以把你的理解进一步压缩成一个非常准确的模型：

```text
第一层：让 OS 活起来
────────────────────
startup
linker
clock
memory
interrupt
tick
RTOS port
scheduler


第二层：让 SoC 外设可用
────────────────────
PinMux
Clock
IRQ
DMA
HAL
OS Device Driver


第三层：让具体板子可用
────────────────────
PHY
Flash
EEPROM
Sensor
LCD
SD Card
Codec
WiFi module
...
```

因此可以把 BSP 理解成：

```text
               BSP
                │
        ┌───────┼────────┐
        │       │        │
     OS启动    SoC外设   板级设备
        │       │        │
   startup    UART      Flash
   linker     SPI       PHY
   clock      CAN       LCD
   memory     ADC       Sensor
   interrupt  DMA       EEPROM
```

一句话：

> **最小 BSP 的目标是让 OS 在这块板上跑起来；完整 BSP 的目标是让板上的硬件资源能够通过 OS 的统一接口被上层软件使用。**

---

# 2. FreeRTOS 最小 BSP

以 Cortex-M + FreeRTOS 为例，最小 BSP 不需要完整设备框架，只需要建立 RTOS 正常运行所依赖的硬件环境。

启动链：

```text
Reset
  ↓
startup.s
  ↓
Vector Table
  ↓
SystemInit()
  ↓
C Runtime
 ├─ copy .data
 └─ clear .bss
  ↓
main()
  ↓
Board/Clock Init
  ↓
FreeRTOS Kernel Init
  ↓
vTaskStartScheduler()
  ↓
SysTick / SVC / PendSV
  ↓
Task Running
```

最小构成包括：

```text
1. startup
   - Reset_Handler
   - Vector Table

2. linker
   - Flash 地址
   - RAM 地址
   - .text/.data/.bss
   - stack / heap

3. clock
   - HSE/HSI
   - PLL
   - SYSCLK / AHB / APB

4. interrupt
   - NVIC
   - interrupt priority

5. RTOS Tick
   - SysTick 或其他硬件 timer

6. Cortex-M RTOS Port
   - SVC
   - PendSV
   - SysTick

7. FreeRTOS 配置
   - FreeRTOSConfig.h
   - tick rate
   - max priority
   - heap
   - interrupt priority

8. RAM / Stack / Heap

9. Scheduler
   - 创建第一个任务
   - vTaskStartScheduler()
```

需要注意：

```text
PendSV/SVC/context switch
```

通常不是 BSP 自己重新实现，而是 FreeRTOS 的 Cortex-M Port 已经提供：

```text
xPortPendSVHandler
vPortSVCHandler
xPortSysTickHandler
```

BSP 的责任主要是：

```text
CPU / Clock / Memory / Interrupt
          ↓
满足 FreeRTOS Port 的运行条件
```

因此：

```text
CPU Port
解决：线程怎么切换

BSP
解决：这块板怎么让 CPU Port 和 Kernel 正常工作
```

---

# 3. 一个新 ARM 板子的 Bring-up 流程

假设拿到：

```text
New ARM Cortex-M Board

已有：
- Datasheet
- Reference Manual
- Schematic
- CMSIS
- startup.s
- Vendor HAL
```

建议严格按照下面顺序 bring-up。

## Stage 1：CPU 能启动

首先验证：

```text
Reset
 ↓
startup
 ↓
main()
```

重点检查：

```text
Vector Table
SP 初始化
PC/Reset_Handler
.data
.bss
linker
```

验收：

```text
main() 能执行
```

---

## Stage 2：Clock

配置：

```text
OSC
 ↓
PLL
 ↓
SYSCLK
 ↓
AHB/APB
```

验收：

```text
CPU 主频正确
GPIO delay / timer 测量正确
```

---

## Stage 3：GPIO

先做最简单 LED：

```text
Clock
 ↓
GPIO
 ↓
LED Toggle
```

验证：

```text
PinMux
GPIO clock
GPIO register/HAL
```

---

## Stage 4：UART

建立最基本调试接口：

```text
UART TX
UART RX
printf
```

验收：

```text
串口稳定输出
```

UART 是后续所有 bring-up 的核心诊断通道。

---

## Stage 5：Interrupt

测试：

```text
Timer IRQ
UART IRQ
GPIO EXTI
```

确认：

```text
Vector
NVIC
priority
ISR
```

全部正确。

---

## Stage 6：Timer / SysTick

建立系统时基：

```text
Hardware Timer
      ↓
System Tick
```

验证：

```text
1ms tick
delay
周期任务
```

---

## Stage 7：RTOS

加入：

```text
FreeRTOS / RT-Thread
```

验证：

```text
Task A
Task B
Scheduler
Semaphore
Timer
```

此时完成：

> 最小 RTOS BSP。

---

## Stage 8：SoC Peripheral

逐项 bring-up：

```text
UART
SPI
I2C
CAN
ADC
PWM
DMA
ETH
USB
SDIO
```

固定流程：

```text
Clock
 ↓
PinMux
 ↓
Peripheral Init
 ↓
IRQ/DMA
 ↓
HAL
 ↓
Functional Test
```

---

## Stage 9：Board Device

再处理板载芯片：

```text
SPI → Flash
I2C → EEPROM/Sensor
RMII → Ethernet PHY
FSMC → SRAM/LCD
SDIO → SD Card
```

流程：

```text
SoC Controller
      ↓
Bus
      ↓
External Device Driver
      ↓
Functional Test
```

---

## Stage 10：产品化验证

最终测试：

```text
Cold Boot
Warm Reset
IRQ Stress
DMA Stress
Multi-thread
Watchdog
Flash
Filesystem
Network
Power Cycle
Exception Recovery
```

所以 ARM Board Bring-up 的整体主线可以记成：

```text
CPU
 ↓
Memory
 ↓
Clock
 ↓
GPIO
 ↓
UART
 ↓
Interrupt
 ↓
Timer
 ↓
RTOS
 ↓
SoC Peripheral
 ↓
Board Device
 ↓
System Integration
```

---

# 4. RT-Thread BSP 的构成

RT-Thread 相比简单 FreeRTOS BSP，多出来的核心就是：

```text
Device Framework
+
Components
+
Configuration / Build System
```

完整模型：

```text
Application
     ↓
RT-Thread Components
 ├─ FinSH
 ├─ DFS
 ├─ FAL
 ├─ SAL
 ├─ lwIP
 └─ ULog
     ↓
RT-Thread Device Framework
 ├─ UART
 ├─ SPI
 ├─ I2C
 ├─ CAN
 ├─ ADC
 ├─ PWM
 └─ ETH
     ↓
SoC Driver
     ↓
Vendor HAL
     ↓
MCU
     ↓
Board Device
```

一个典型 BSP 主要包含：

```text
bsp/my-board/

├─ applications/
│
├─ board/
│  ├─ board.c
│  ├─ board.h
│  ├─ Kconfig
│  ├─ linker_scripts/
│  └─ ports/
│
├─ Kconfig
├─ SConscript
├─ SConstruct
├─ rtconfig.py
└─ README.md
```

各部分职责：

```text
board.c
→ clock / heap / board init

board.h
→ memory / peripheral / board definition

linker_scripts
→ Flash/RAM layout

ports/
→ 外部设备板级适配

Kconfig
→ 功能选择

SConscript/SConstruct
→ 构建关系

HAL Drivers
→ SoC 外设适配

Device Framework
→ 提供统一设备接口
```

---

# 5. RT-Thread BSP 实际开发流程

对于一个新的 Cortex-M 板，实际建议按照下面流程。

```text
① 确认 CPU Port
```

例如 Cortex-M4 已经支持：

```text
libcpu/arm/cortex-m4
```

那么不要重新实现 context switch。

↓

```text
② Vendor 裸机工程跑通
```

至少：

```text
startup
clock
GPIO
UART
IRQ
```

↓

```text
③ 找最接近的 RT-Thread BSP/template
```

优先：

```text
同 SoC
> 同系列
> 同 Cortex Core
```

↓

```text
④ 建立新 BSP Skeleton
```

↓

```text
⑤ 修改 startup / linker / memory
```

↓

```text
⑥ 配置 System Clock
```

↓

```text
⑦ 跑 RT-Thread Kernel
```

先验证：

```text
scheduler
thread
tick
```

↓

```text
⑧ UART + GPIO + FinSH
```

达到：

```text
msh >
```

这是 Minimal RT-Thread BSP 的关键验收点。

↓

```text
⑨ 接入 SoC Driver Framework
```

例如：

```text
HAL UART
   ↓
drv_usart
   ↓
RT-Thread Serial Framework
   ↓
uart1 device
```

↓

```text
⑩ 接入 SPI/I2C/CAN/DMA 等
```

固定模式：

```text
PinMux
Clock
IRQ
DMA
HAL
RT-Thread Driver
rt_device
```

↓

```text
⑪ 接入板载 Device
```

例如：

```text
SPI1
 ↓
spi10
 ↓
W25Q128
 ↓
SFUD
 ↓
FAL
 ↓
DFS / OTA
```

↓

```text
⑫ Kconfig / SCons 完善
```

最终要求：

```text
menuconfig
 ↓
Enable device
 ↓
Build
 ↓
Flash
 ↓
list_device
 ↓
直接使用
```

而不是每次启用外设都手工改大量源码。

---

# 6. BSP 开发的核心

最终可以把 BSP 开发压缩为三个词：

```text
Bring-up
+
Abstraction
+
Integration
```

### Bring-up

解决：

```text
CPU
startup
clock
memory
interrupt
tick
```

目标：

> OS 能稳定运行。

### Abstraction

解决：

```text
HAL
 ↓
OS Driver
 ↓
Device Framework
```

目标：

> 上层应用不关心具体 MCU。

### Integration

解决：

```text
SoC
+
Board Device
+
Kconfig
+
Build
+
Middleware
```

目标：

> 整块板成为一个可配置、可复用、可维护的平台。

最终最重要的心智模型就是：

```text
Hardware
   ↓
Vendor HAL
   ↓
SoC Driver
   ↓
OS Device Framework
   ↓
OS Component
   ↓
Middleware
   ↓
Application
```

而 BSP 主要负责把下面这一段打通：

```text
Hardware
   ↕
HAL
   ↕
Driver
   ↕
OS
```

因此：

> **BSP 的核心不是“写驱动”，而是完成具体硬件平台到操作系统之间的启动、抽象和集成。**

真正困难的地方通常集中在：

```text
startup ↔ linker
clock ↔ peripheral
IRQ ↔ scheduler
DMA ↔ cache/memory
HAL ↔ OS Device Model
SoC ↔ Board
Bootloader ↔ Application
```

这也是 BSP 工程师相对于普通 MCU 外设驱动工程师真正增加的系统能力。