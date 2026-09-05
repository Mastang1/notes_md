# Source Index

本文档给整个课程提供稳定 source ID。每日教程只需写 source ID + 章节/页码 + 阅读目标。

## 本地/厂商资料

### SRC-F407-SCH

**Explorer STM32F4_V2.2_SCH.pdf**（用户上传）

- p.1：LAN8720A Ethernet + audio；
- p.2：STM32F407ZET6、HSE/LSE、JTAG、USART1、I/O 主网表；
- p.3：W25Q128、LED/KEY、CAN/RS485/SD 等；
- p.4：CH340G USB-UART、电源；
- p.5：PCB placement。

本地：`../references/Explorer STM32F4_V2.2_SCH.pdf`


### SRC-ST-F407-DATASHEET

STMicroelectronics, **STM32F405xx / STM32F407xx Datasheet, DS8626 Rev 12**：
https://www.st.com/resource/en/datasheet/stm32f407ze.pdf

本课程已核实的 exact-part 信息：

- Ordering information，PDF p.186：`Z` = 144 pins；`E` = **512 Kbytes Flash**；`T` = LQFP；`6` = -40~85 °C；
- Table 2 / Embedded SRAM：system SRAM **192 KB（112+16+64 KB）**，另有 **4 KB backup SRAM**。

用途：Explorer F407 board port、memory DTS、linker/MCUboot image layout。

### SRC-WINBOND-W25Q128

Winbond W25Q128 Serial NOR 官方资料入口：
https://www.winbond.com/hq/support/documentation/?__locale=en&pno=W25Q128JV

Winbond selection guide 将 W25Q128 列为 **128 M-bit** 器件，即 16 MiB 地址容量等级。Explorer 原理图只写 `W25Q128`，没有给完整 silicon revision/package suffix，因此课程**不提前锁死 erase/program geometry**；进入 MCUboot 分区设计前需读取实物丝印并匹配对应 datasheet。

用途：外部 SPI NOR 容量核实、后续 MCUboot secondary/staging 与擦除粒度设计。

### SRC-IMX6ULL-APP

**《I.MX6U嵌入式Linux C应用编程指南V1.1》**

公开归档：
https://github.com/alientek-openedv/imx6ull-document

第一章用于建立 syscall/library/application 与驱动层边界；提高篇采用 ARM gcc 交叉编译开发板程序。

### SRC-IMX6ULL-DRV

**《I.MX6U嵌入式Linux驱动开发指南V1.5.2》**

公开归档：
https://github.com/alientek-openedv/imx6ull-document

已能从公开连载核对的章节：

- 第 3 章：Ubuntu/Linux 基础与 GCC 相关内容（具体小节以本地 PDF 目录为准）；
- 第 4 章：开发环境搭建；
- 第 43 章：Linux 设备树；
- 第 55 章：设备树下的 platform 驱动。

> 由于当前对话没有上传该 95.7 MB PDF，本批不伪造精确 PDF 页码；教程会给出**章节 + 公开仓库 + 阅读目标**。你把本地 PDF 放入 `references/` 后可直接打开。

### SRC-IMX6ULL-TFTP-NFS

**《I.MX6U网络环境TFTP&NFS搭建手册V1.3.1》**

公开归档同上。公开资料可核实其包含电脑/虚拟机/开发板的网络拓扑与 TFTP/NFS 搭建。Day 4 以当前 Ubuntu 24.04 服务管理方式为主，厂商手册用于核对板端拓扑与 U-Boot 习惯。

### SRC-NXP-IMX6ULL-RM

NXP i.MX 6ULL Applications Processor Reference Manual。

用途：后续 IOMUXC、GPIO、UART、clock、interrupt、DMA 等硬件寄存器事实。

## Linux 官方

### SRC-LINUX-DT

Linux and the Devicetree:
https://docs.kernel.org/devicetree/usage-model.html

重点：early scan、`unflatten_device_tree()`、device population、`of_platform_populate()`。

### SRC-LINUX-DT-BINDING

Writing Devicetree Bindings in json-schema:
https://docs.kernel.org/devicetree/bindings/writing-schema.html

重点：Linux DT binding 使用 JSON Schema vocabulary + YAML 表示，约束 properties/required/compatible 等。

### SRC-LINUX-DRIVER-MODEL

Driver Model / Platform Devices and Drivers:
https://docs.kernel.org/driver-api/driver-model/platform.html

Driver Binding:
https://docs.kernel.org/driver-api/driver-model/binding.html

重点：bus match、`struct device`、`platform_device`、driver registration、probe。

## Zephyr 官方

### SRC-ZEPHYR-GETTING-STARTED

https://docs.zephyrproject.org/latest/develop/getting_started/

本课程固定 **Zephyr v4.4.1**（2026-06 发布的 4.4 维护版本），而不是跟随 main。

### SRC-ZEPHYR-F4-DISCO

https://docs.zephyrproject.org/latest/boards/st/stm32f4_disco/doc/index.html

官方 target：`stm32f4_disco/stm32f407xx`。它证明 `stm32f407xx` SoC 已有成熟上游支持，但板级 pin/clock/peripheral 不能照抄到 Explorer。

### SRC-ZEPHYR-DT

https://docs.zephyrproject.org/latest/build/dts/

用途：DTS、bindings、generated headers、`DT_*` 宏、build-time hardware description。

### SRC-MCUBOOT

https://docs.mcuboot.com/

本批只建立来源，MCUboot/OTA 正式实战在后续周次。
