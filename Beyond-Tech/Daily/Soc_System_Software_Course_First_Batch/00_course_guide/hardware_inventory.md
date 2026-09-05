# Hardware Inventory - 第一批已验证硬件事实

## 1. 正点原子 STM32F4 Explorer

本表只使用用户上传的 `Explorer STM32F4_V2.2_SCH.pdf`。任何与其他 F407 开发板不同的地方，以该原理图为准。

| 资源 | 已核实事实 | 原理图 | 本课程用途 |
|---|---|---|---|
| MCU | **STM32F407ZET6**，LQFP144 | p.2 U4 | Zephyr custom board SoC |
| Internal Flash | **512 KB**（order code 中 `E` = 512 KB） | ST DS8626 Rev 12 ordering information | Zephyr/MCUboot image layout 基线 |
| System SRAM | **192 KB = 112 + 16 + 64 KB CCM**；另有 4 KB backup SRAM | ST DS8626 Rev 12 Table 2 / SRAM section | stack/heap/buffer/MCUboot RAM 预算 |
| HSE | 8 MHz 晶振 Y2 | p.2 | board clock |
| LSE | 32.768 kHz Y3 | p.2 | RTC/低速时钟 |
| USART1 | PA9=TX, PA10=RX；通过 **P6 2x2 jumper/header** 与板载 `TXD/RXD` 链路相连 | p.2 | Zephyr console 候选；先确认 P6 跳帽连接 |
| USB-UART | CH340G | p.4 U17 | PC console 通道 |
| LED0 | PF9；LED+电阻接 3.3 V，GPIO 灌电流，**active-low** | p.2/p.3 | board smoke test |
| LED1 | PF10；LED+电阻接 3.3 V，GPIO 灌电流，**active-low** | p.2/p.3 | board smoke test |
| KEY0 | PE4；按键闭合到 GND，**active-low** | p.2/p.3 | GPIO input test |
| KEY1 | PE3；按键闭合到 GND，**active-low** | p.2/p.3 | GPIO input test |
| KEY2 | PE2；按键闭合到 GND，**active-low** | p.2/p.3 | GPIO input test |
| SPI1 SCK | PB3 | p.2/p.3 | W25Q128 |
| SPI1 MISO | PB4 | p.2/p.3 | W25Q128 |
| SPI1 MOSI | PB5 | p.2/p.3 | W25Q128 |
| W25Q128 CS | `F_CS`, net traced to PB14 | p.2/p.3 | 外部升级/存储候选 |
| SPI NOR | W25Q128，容量等级 **128 Mbit = 16 MiB**；具体后缀/erase geometry 待读芯片丝印 + 对应 Winbond datasheet | p.3 U11 + Winbond family docs | 后续 MCUboot secondary/staging 候选 |
| Ethernet PHY | LAN8720A, RMII | p.1 U1 | 本批延后，不做网络栈 |
| Debug | 20-pin JTAG，含 SWDIO/SWCLK/SWO/RESET | p.2 | OpenOCD/JTAG 调试 |

直接链接：

- [p.1 Ethernet/Audio](../references/Explorer%20STM32F4_V2.2_SCH.pdf#page=1)
- [p.2 MCU/Core/JTAG/Clock](../references/Explorer%20STM32F4_V2.2_SCH.pdf#page=2)
- [p.3 Peripherals/W25Q128/LED/KEY](../references/Explorer%20STM32F4_V2.2_SCH.pdf#page=3)
- [p.4 CH340/Power](../references/Explorer%20STM32F4_V2.2_SCH.pdf#page=4)
- [p.5 PCB placement](../references/Explorer%20STM32F4_V2.2_SCH.pdf#page=5)

> 注意：这里已经按 **exact part `STM32F407ZET6`** 核实：`Z`=144 pins，`E`=512 KB Flash，`T`=LQFP，`6`=-40~85 °C 工业温区。ST 数据手册还给出该系列 system SRAM 为 192 KB（112+16+64 KB，其中 64 KB 为 CCM）并另有 4 KB backup SRAM。后续 MCUboot layout 仍需再核对 **Zephyr 当前 SoC DTS 的 memory/flash 定义** 与实际 image size，不能仅凭总容量划分分区。

官方：<https://www.st.com/resource/en/datasheet/stm32f407ze.pdf>，Ordering information（PDF p.186）与 Table 2/Embedded SRAM。

## 2. i.MX6ULL

第一批只把 i.MX6ULL 当作 Linux Target：

- 串口进入 U-Boot / Linux console；
- Ethernet 与 Ubuntu VM 同网段；
- TFTP/NFS；
- 后续 Kernel/DTB/Driver。

具体板卡型号、DDR/eMMC/NAND 版本在 Day 3 的 `board_inventory.md` 中现场确认，避免教程替你猜。
