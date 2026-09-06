# Week 2 Reference Manual Index

## 使用规则

Week 2 的 Linux 部分优先按正点原子公开资料的实际版本和实际操作流程写；Zephyr 部分固定以 **Zephyr v4.4.1** 源码结构为准。

I.MX6ULL 的大型 PDF 当前没有作为本会话附件上传，因此本教程：
- 给出手册准确名称和版本；
- 给出已核实章节/小节；
- 给出公开在线链接；
- 不编造 PDF 物理页码。

Explorer STM32F4 原理图是实际上传文件，因此可直接引用 PDF 页码。

| ID | 资料 | Week 2 使用位置 |
|---|---|---|
| ALI-QUICK-1.8 | I.MX6U 用户快速体验 V1.8 | 1.2 软件资源、4.2 Poky、4.3 U-Boot、4.4 Kernel/DTB/modules |
| ALI-DRV-1.5.2 | I.MX6U 嵌入式 Linux 驱动开发指南 V1.5.2 | BSP/交叉工具链/U-Boot/Linux |
| ALI-NET-1.3.1 | I.MX6U 网络环境 TFTP&NFS 搭建手册 V1.3.1 | TFTP 网络启动 |
| F407-SCH-2.2 | Explorer STM32F4 V2.2 Schematic | p.2 MCU/USART1/clock；p.3 LED/W25Q128；p.4 CH340 |
| ZEPHYR-PORT | Zephyr Board Porting Guide | Hardware model v2 |
| ZEPHYR-CUSTOM | Custom Board definitions | `BOARD_ROOT` / out-of-tree board |
| ZEPHYR-FLASH | west flash/debug | runner / flash / debug |

## 正点原子本周关键章节

### I.MX6U 用户快速体验 V1.8
- **1.2 软件资源简介**
  - 出厂 U-Boot：2016.03
  - 出厂 Linux：4.1.15
  - 通用 ARM GCC：4.9.4
  - Poky GCC：5.3.0
- **4.2** 安装 Poky 交叉编译工具链
- **4.3** 编译出厂 U-Boot
- **4.4** 编译出厂 Linux Kernel + DTB + modules

在线：
- [Software resources](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/introduction%20to%20software%20and%20hardware%20resources/Introduction%20to%20Software%20Resources/)
- [Chapter 4 index](https://wiki.alientek.com/docs/category/%E7%AC%AC%E5%9B%9B%E7%AB%A0-%E4%BA%A4%E5%8F%89%E7%BC%96%E8%AF%91-1/)
- [4.3 U-Boot](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/u-boot/)
- [4.4 Kernel](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/comple_core/)

### Zephyr v4.4.1 近似参考板

Explorer 使用 `STM32F407ZET6`。Zephyr v4.4.1 中 `black_f407ve` 同属 STM32F407XE，可参考：
- `board.yml` 中 SoC 名 `stm32f407xx`
- `Kconfig.black_f407ve` 中 `select SOC_STM32F407XE`
- `stm32f407Xe.dtsi`
- USART1 PA9/PA10、115200
- 8 MHz HSE -> PLL -> 168 MHz

只参考 SoC/clock/UART 组织方式，不复制其 LED/SPI pin。

- [board.yml](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/board.yml)
- [Kconfig](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/Kconfig.black_f407ve)
- [defconfig](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/black_f407ve_defconfig)
- [DTS](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/black_f407ve.dts)

## 本地原理图

- [Explorer STM32F4 V2.2 schematic](Explorer_STM32F4_V2.2_SCH.pdf)
