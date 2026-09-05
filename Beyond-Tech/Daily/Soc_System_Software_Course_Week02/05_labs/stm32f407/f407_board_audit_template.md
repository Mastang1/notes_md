# Explorer STM32F407 Board Audit

> 原则：没有 schematic/datasheet/official binding 证据的字段不凭记忆填写。

| Resource | Schematic Page | Net | MCU Pin | AF/Mode | Zephyr Node/Binding Candidate | Driver/Subsys | Verification |
|---|---:|---|---|---|---|---|---|
| MCU | 2 | U4 | STM32F407ZET6 | - | stm32f407xx | SoC | schematic |
| HSE | 2 | Y2 | PH0/PH1 | OSC | hse | clock control | schematic |
| LSE | 2 | Y3 | PC14/PC15 | OSC32 | lse | clock control | schematic |
| USART1 TX | 2 | USART1_TX ↔ P6 ↔ RXD | PA9 | verify AF | &usart1 | serial | schematic + jumper + datasheet |
| USART1 RX | 2 | USART1_RX ↔ P6 ↔ TXD | PA10 | verify AF | &usart1 | serial | schematic + jumper + datasheet |
| LED0 | 2/3 | LED0 | PF9 | GPIO, active-low | gpio-leds | gpio | schematic |
| LED1 | 2/3 | LED1 | PF10 | GPIO, active-low | gpio-leds | gpio | schematic |
| KEY0 | 2/3 | KEY0 | PE4 | GPIO input, active-low | gpio-keys | gpio | schematic |
| KEY1 | 2/3 | KEY1 | PE3 | GPIO input, active-low | gpio-keys | gpio | schematic |
| KEY2 | 2/3 | KEY2 | PE2 | GPIO input, active-low | gpio-keys | gpio | schematic |
| W25Q128 CS | 2/3 | F_CS | PB14 | GPIO output | jedec,spi-nor | flash | schematic |
| SPI1 SCK | 2/3 | SPI1_SCK | PB3 | verify AF | &spi1 | spi | schematic + datasheet |
| SPI1 MISO | 2/3 | SPI1_MISO | PB4 | verify AF | &spi1 | spi | schematic + datasheet |
| SPI1 MOSI | 2/3 | SPI1_MOSI | PB5 | verify AF | &spi1 | spi | schematic + datasheet |
| JTAG/SWD | 2 | JTAG | PA13/PA14/... | SWD/JTAG | debug | OpenOCD | schematic |
| LAN8720 | 1 | RMII_* | multiple | ETH AF | deferred | ethernet | schematic |

## Exact part memory audit

- MCU full order code: STM32F407ZET6
- Internal Flash: **512 KB**（ST DS8626 Rev 12 ordering information：`E=512 Kbytes`）
- System SRAM: **192 KB = 112 + 16 + 64 KB CCM**
- Backup SRAM: **4 KB**
- External W25Q128 capacity: **128 Mbit = 16 MiB**（Winbond family docs）
- External W25Q128 exact suffix / erase geometry: **读取实物丝印后匹配 datasheet 再填**

## Console chain

```text
PC USB
→ CH340G (p.4)
→ TXD/RXD nets
→ P6 2x2 jumper/header (p.2)
→ USB_UART/USART1 block (p.2)
→ PA9/PA10
→ USART1
```

## Board-port entry criteria

- [x] exact MCU internal Flash/SRAM confirmed from ST DS8626 Rev 12
- [ ] HSE/LSE confirmed
- [ ] P6 jumper installed/verified and console TX/RX direction confirmed
- [ ] LED/KEY pins confirmed
- [ ] SPI NOR pin/CS confirmed
- [ ] upstream same-SoC build baseline passes
