# Week 03 - Linux User-Space System Model + Zephyr Basic Peripherals

## Week goal

- Explain a Linux user program from **ELF → process → virtual address → fd → syscall** with tools/evidence.
- Complete Explorer F407 LED/KEY Devicetree and a hardware `west debug` session.

## Daily tutorials

| Day | Tutorial | Main output | Manual / official reading |
|---|---|---|---|
| D1 | [Process and Syscall](linux/W03D01_Process_and_Syscall.md) | fork/exec/wait + strace | C App Guide Ch.1 + Ch.9 (§9.1/9.5/9.10/9.11) |
| D2 | [ELF and Linking](linux/W03D02_ELF_and_Linking.md) | section/segment/symbol map | C App Guide §1.4, §9.3 + `elf(5)` |
| D3 | [Virtual Address Space](linux/W03D03_Virtual_Address_Space.md) | `/proc/pid/maps` evidence | C App Guide §9.3/§9.4 |
| D4 | [File Descriptor and UAPI](linux/W03D04_File_Descriptor_and_UAPI.md) | reusable `user_tool.c` | C App Guide Ch.2 + advanced I/O keywords |
| D5 | [Zephyr LED/KEY Devicetree](zephyr/W03D05_LED_KEY_Devicetree.md) | real LED/KEY DTS | Explorer schematic p.2/3 + Zephyr GPIO bindings |
| D6 | [west Debug and GDB](zephyr/W03D06_West_Debug_and_GDB.md) | breakpoint/register session | Explorer schematic p.2 + Zephyr west debug + PM0214 |
| D7 | [Week 3 Feynman Gate](linux/W03D07_Week3_Feynman_Gate.md) | 10-minute oral outline | targeted re-read only |

## Manuals and direct links

- [ALIENTEK i.MX6ULL Linux C Application Guide V1.1 - local](references/ALIENTEK_iMX6ULL_Linux_C_Application_Programming_Guide_V1.1.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- [ALIENTEK i.MX6ULL Linux Driver Guide V1.5.2 - local](references/ALIENTEK_iMX6ULL_Linux_Driver_Development_Guide_V1.5.2.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- [Explorer STM32F4 schematic](references/Explorer_STM32F4_V2.2_SCH.pdf)
- [ST RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [Zephyr GPIO](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [Zephyr west debug](https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html)
- [Source index](references/source_index.md)
- [Local manual filename mapping](references/README.md)
