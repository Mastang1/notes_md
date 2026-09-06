# Part I - Establish the Development Loop

> Week 1 的核心问题：**怎样建立一个可以长期支撑 Linux BSP + Zephyr 实战的双平台开发闭环？**

这不是“环境搭建周”。知识链是严格递进的：

```mermaid
flowchart LR
    H[Host] --> C[Cross Compile]
    C --> T[Target Observation]
    T --> D[Fast Deploy]
    D --> Z[Zephyr Build World]
    Z --> B[Real Board Audit]
    B --> R[Clean Reproduction]
```

## Reading order

- [Chapter 1 - Build a Reproducible Embedded Linux Development Host](ch01_linux_host.md) - 开发主机/网络/环境基线
- [Chapter 2 - Cross Compilation: From C Source to ARM ELF](ch02_cross_compilation_elf.md) - 交叉工具链/ELF
- [Chapter 3 - First Contact with i.MX6ULL](ch03_imx6ull_console_network.md) - 串口/U-Boot/Linux/网络
- [Chapter 4 - Shorten the Development Cycle](ch04_tftp_nfs_loop.md) - TFTP/NFS 开发闭环
- [Chapter 5 - Understand the Zephyr Build World](ch05_zephyr_build_world.md) - west/Kconfig/Devicetree
- [Chapter 6 - Hardware Before Software](ch06_f407_hardware_audit.md) - F407 原理图/Board Audit
- [Chapter 7 - Integration](ch07_week1_integration.md) - Clean-state 复现

## Week 1 final capability

完成本 Part 后，你应该能够在**不依赖聊天记录**的情况下：建立 Ubuntu Host、生成 ARM ELF、从串口观察 6ULL 启动、配置 TFTP/NFS、构建官方 Zephyr STM32F4 sample，并从用户上传原理图准确审计 STM32F407 Explorer 的 clock/UART/LED/KEY/SPI Flash。

## Manuals and primary references

- [ALIENTEK i.MX6ULL VM Guide V1.2](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E8%99%9A%E6%8B%9F%E6%9C%BA%E4%BD%BF%E7%94%A8%E5%8F%82%E8%80%83%E6%89%8B%E5%86%8CV1.2.pdf)
- [ALIENTEK i.MX6ULL Linux C Application Guide V1.1](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- [ALIENTEK i.MX6ULL TFTP & NFS Guide V1.3.1](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%BD%91%E7%BB%9C%E7%8E%AF%E5%A2%83TFTP%26NFS%E6%90%AD%E5%BB%BA%E6%89%8B%E5%86%8CV1.3.1.pdf)
- [ALIENTEK i.MX6ULL Quick Start V1.8](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%94%A8%E6%88%B7%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8CV1.8.pdf)
- [Local STM32F4 Explorer V2.2 schematic](../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf)
- [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [Unified source index](../common/source_index.md)

## Time budget

每章核心学习块约 2 小时。安装/下载等待时间不计入主动学习；遇到下载耗时可在后台进行，但不要趁等待跳到无关知识点。

## Manual reading map

| Manual | Local expected filename | Online | This Part uses |
|---|---|---|---|
| I.MX6U Virtual Machine Guide V1.2 | `ALIENTEK_iMX6ULL_VM_Guide_V1.2.pdf` | [Open archive](https://github.com/alientek-openedv/imx6ull-document) | Ch01: VM/Ubuntu baseline |
| I.MX6U Linux C Application Guide V1.1 | `ALIENTEK_iMX6ULL_Linux_C_Application_Guide_V1.1.pdf` | [Open PDF page](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf) | Ch02: cross compile/ELF |
| I.MX6U TFTP & NFS Guide V1.3.1 | `ALIENTEK_iMX6ULL_TFTP_NFS_Guide_V1.3.1.pdf` | [Open archive](https://github.com/alientek-openedv/imx6ull-document) | Ch04: TFTP/NFS |
| Explorer STM32F4 V2.2 Schematic | packaged | [Local PDF](../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf) | Ch06: MCU/clock/UART/LED/KEY/W25Q128 |

章节号/页码如果不同，以你实际 PDF 版本的章节标题为准；本课程不会为了“看起来精确”伪造页码。

