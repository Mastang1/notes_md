# F407 Explorer Board Audit

| Resource | Schematic page | Net / chip | MCU pin | Active level / AF | Zephyr object later | Verification |
|---|---:|---|---|---|---|---|
| MCU | p.2 | STM32F407ZET6 | - | - | SoC | |
| HSE | p.2 | Y2 8 MHz | PH0/PH1 | oscillator | clocks | |
| LSE | p.2 | Y3 32.768 kHz | PC14/PC15 | oscillator | clocks | |
| USART1 TX | p.2/p.4 | USART1_TX -> P6 -> TXD/RXD -> CH340G | trace | AF7 | console | |
| USART1 RX | p.2/p.4 | USART1_RX -> P6 -> TXD/RXD -> CH340G | trace | AF7 | console | |
| LED0 | p.2/p.3 | LED0 | PF9 | active-low | led0 alias | |
| LED1 | p.2/p.3 | LED1 | PF10 | active-low | led1 | |
| KEY0 | p.2/p.3 | KEY0 | trace | trace | gpio-keys | |
| KEY1 | p.2/p.3 | KEY1 | trace | trace | gpio-keys | |
| W25Q128 CS | p.3 | F_CS | trace | GPIO | spi-nor cs | |
| W25Q128 SCK | p.3 | SPI1_SCK | PA5 | AF5 | spi1 | |
| W25Q128 MISO | p.3 | SPI1_MISO | PA6 | AF5 | spi1 | |
| W25Q128 MOSI | p.3 | SPI1_MOSI | PA7 | AF5 | spi1 | |
| Ethernet | p.1 | LAN8720A | RMII nets | AF | ethernet | deferred |
| SWD/JTAG | p.2 | JTAG header | JTMS/JTCK... | AF0 | debug | |
