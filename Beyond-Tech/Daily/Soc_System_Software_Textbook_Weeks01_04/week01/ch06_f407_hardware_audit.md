# Chapter 6 - Hardware Before Software: Audit the STM32F407 Explorer Board

> Week 1 / Day 6 - 自定义 Zephyr board port 的板级事实必须先审计。

[← Part README](README.md) · [← Previous](ch05_zephyr_build_world.md) · [Next →](ch07_week1_integration.md)

## 6.1 Board Port 的第一原则：任何 pin/clock 都必须能回到原理图

“同样是 STM32F407”不等于“板级配置可以复制”。SoC 决定有哪些外设 IP，PCB 决定晶振、串口、LED、Flash 接到了哪个 pin。

费曼模型：

> Zephyr 已经知道“STM32F407 是什么芯片”；你要告诉它“正点原子这块 PCB 是怎样把这个芯片接出去的”。

## 6.2 先确认主角：U4 = STM32F407ZET6

以本包中的上传原理图为权威来源：[Explorer schematic](../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf)。

原理图 p.2 标明 U4 为 `STM32F407ZET6`。因此不要再按常见 Discovery `STM32F407VGT6` 或其他“F407 板”假设 Flash/封装/pin。

### 时钟

p.2：

- HSE：Y2 = 8 MHz；
- LSE：Y3 = 32.768 kHz。

后续 `clocks`、PLL source 的配置必须从这里出发。

## 6.3 Console 链路：PA9/PA10 -> USART1 -> P6 -> CH340G -> USB

p.2 MCU pin 表可看到：

- PA9 = USART1_TX；
- PA10 = USART1_RX；
- 板级 net `USART1_TX/USART1_RX` 引到 P6。

p.4 CH340G 提供 USB-UART。你需要实际检查跳帽/连接状态，确认 MCU USART1 net 与 CH340 的 TXD/RXD 路径连通。

这直接决定 Week 2 自定义 board 中：

```dts
chosen {
    zephyr,console = &usart1;
};
```

以及 USART1 pinctrl。

## 6.4 最小 Smoke Test 外设：LED 与 KEY

根据 p.2/p.3 网络：

- LED0 -> PF9；
- LED1 -> PF10；
- KEY0 -> PE4；
- KEY1 -> PE3；
- KEY2 -> PE2；
- WK_UP -> PA0。

LED 电路由 VCC3.3 经限流电阻/LED 接到 MCU net，因此常见行为是 GPIO 拉低点亮，后续 DTS 应通过 `GPIO_ACTIVE_LOW` 表达电气语义，而不是应用里到处写 `!value`。

这就是 Devicetree 的价值之一：**把板级极性放在硬件描述，而不是业务逻辑。**

## 6.5 W25Q128：后续 MCUboot/升级项目最有价值的板载资源

p.3 U11 = W25Q128：

- SPI1_SCK；
- SPI1_MISO；
- SPI1_MOSI；
- `F_CS`；
- 3.3 V。

p.2 对应 MCU 复用网络中 SPI1 常用 PB3/PB4/PB5；`F_CS` 也必须从网名继续追到 MCU pin，最终 board audit 表里写死，不凭印象。

本课程 Week 1 只做硬件审计，不马上做 Flash driver/MCUboot。

## 6.6 LAN8720A：知道它在板上，但现在主动不学

p.1 是 LAN8720A RMII PHY。它未来可以用于 Ethernet/UDP transport，但当前阶段目标是 Zephyr board bring-up 与 Bootloader/DFU 基础。不要因为“板上有网口”就提前打开 networking 这条支线。

这叫 scope control：知道资源存在，但只有主线需要时才引入。

## 6.7 Guided Lab：创建真正的 Board Audit Table

建立 `f407_board_audit.md`：

| Resource | Schematic | Net | MCU pin | Electrical note | Zephyr object | Verified |
|---|---|---|---|---|---|---|
| HSE | p.2 | OSC_IN/OUT | PH0/PH1 | 8MHz | clocks | |
| USART1 TX | p.2/p.4 | USART1_TX | PA9 | to CH340 path | usart1 pinctrl | |
| USART1 RX | p.2/p.4 | USART1_RX | PA10 | to CH340 path | usart1 pinctrl | |
| LED0 | p.2/p.3 | LED0 | PF9 | active-low | gpio-leds | |
| W25Q128 | p.3 | SPI1_* | trace | 3.3V | jedec,spi-nor | |

**Verified 列必须来自原理图或上板测量。**

## 6.8 Independent Challenge：抓一个“别的 F407 教程”来反证为什么不能抄

任选 STM32F4 Discovery 的 LED/UART pin，与本板对比。写出至少三处板级差异。

你要形成条件反射：看到 `compatible = "st,stm32f407"` 只能说明 SoC 类似，不能推出 LED、console、HSE、external flash 一样。

## 6.9 本章小结与下一章

Week 1 到这里第一次完成了“软件配置回到真实原理图”的闭环。Day 7 不继续加知识，而是从新 shell/clean state 重做关键动作，验证这些环境与事实是否真正属于你，而不是属于聊天记录。

## References and manuals

### STM32F407 Explorer V2.2 Schematic
- Local expected path: `../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf`

- 本章阅读定位：本章权威来源：p.2 MCU/clock/JTAG/UART，p.3 W25Q128/LED/KEY，p.4 CH340G，p.1 LAN8720A。

### Zephyr Board Porting Guide

- Online: [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- 本章阅读定位：先看 board/SoC/DTS/Kconfig 文件的职责，不要求今天创建 board。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch05_zephyr_build_world.md) · [Next →](ch07_week1_integration.md)
