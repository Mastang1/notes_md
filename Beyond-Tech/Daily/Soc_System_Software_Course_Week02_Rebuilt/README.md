# Week 02 - BSP Build Loop + STM32F407 Zephyr Board Port

## Week goal

- i.MX6ULL: understand BSP artifact locations, independently build U-Boot/Kernel/DTB, boot Kernel+DTB from RAM over TFTP, complete one remote GDB session.
- STM32F407: create an out-of-tree Zephyr board, configure clock/USART1 console, complete first real-board output/debug path.

## Daily tutorials

| Day | Tutorial                                                                             | Core outcome                               | Primary manual reading                                 |
| --- | ------------------------------------------------------------------------------------ | ------------------------------------------ | ------------------------------------------------------ |
| D1  | [BSP Artifact Map](linux/W02D01_iMX6ULL_BSP_Artifact_Map.md)                         | Source/config/artifact/target map          | Driver Guide Ch.30 + Ch.35 §35.2/35.3                  |
| D2  | [Full Build U-Boot/Kernel/DTB](linux/W02D02_iMX6ULL_Full_Build_UBoot_Kernel_DTB.md)  | Reproducible full build                    | Driver Guide Ch.30 §30.2 + Ch.35 §35.2/35.3            |
| D3  | [TFTP Boot Kernel and DTB](linux/W02D03_TFTP_Boot_Kernel_and_DTB.md)                 | RAM-only replacement loop                  | Driver Guide Ch.30 (`tftp`,`bootz`) + TFTP/NFS Guide   |
| D4  | [Remote GDB User-space Debug](linux/W02D04_Remote_GDB_User_Space_Debugging.md)       | Host GDB ↔ target gdbserver                | C App Guide Ch.1 + GNU GDB manual                      |
| D5  | [Zephyr Out-of-Tree Board](zephyr/W02D05_Zephyr_Out_of_Tree_Board_Creation.md)       | Custom board discover/build                | Explorer schematic p.2/3/4 + Zephyr Board Porting      |
| D6  | [F407 Clock/Console/Flashing](zephyr/W02D06_STM32F407_Clock_Console_and_Flashing.md) | Real UART console                          | Explorer schematic p.2/4 + RM0090 + Zephyr flash/debug |
| D7  | [Week 2 Clean Build Gate](linux/W02D07_Week2_Clean_Build_and_DT_Comparison.md)       | Reproducibility + Linux/Zephyr DT contrast | Driver Guide Ch.35 + official DT docs                  |

## Deep Dive

- [DeviceTree: From DTS to Linux Device](deep_dive/A01_DeviceTree_From_DTS_to_Linux_Device.md)

## Manuals and direct links

- [ALIENTEK i.MX6ULL Linux Driver Development Guide V1.5.2 - local](references/ALIENTEK_iMX6ULL_Linux_Driver_Development_Guide_V1.5.2.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- [ALIENTEK i.MX6ULL Linux C Application Guide V1.1 - local](references/ALIENTEK_iMX6ULL_Linux_C_Application_Programming_Guide_V1.1.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- [ALIENTEK i.MX6ULL TFTP & NFS Guide V1.3.1 - local](references/ALIENTEK_iMX6ULL_TFTP_NFS_Setup_Guide_V1.3.1.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%BD%91%E7%BB%9C%E7%8E%AF%E5%A2%83TFTP%26NFS%E6%90%AD%E5%BB%BA%E6%89%8B%E5%86%8CV1.3.1.pdf)
- [Explorer STM32F4 V2.2 schematic](references/Explorer_STM32F4_V2.2_SCH.pdf)
- [ST RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- [Zephyr Devicetree Guide](https://docs.zephyrproject.org/latest/build/dts/index.html)
- [Source index](references/source_index.md)
- [Reference setup / filename mapping](references/README.md)

> If the local ALIENTEK PDF links are broken, copy your existing PDFs into `references/` using the ASCII filenames listed in [references/README.md](references/README.md). The online links work immediately.
