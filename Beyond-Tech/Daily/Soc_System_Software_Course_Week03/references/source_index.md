# Source Index

This file is the stable reference registry for the course packages. All package/file names are ASCII-only.

| Source ID | Manual / Documentation | Expected local file | Public link | Main use |
|---|---|---|---|---|
| `SRC-IMX6ULL-DRV` | ALIENTEK I.MX6U Embedded Linux Driver Development Guide V1.5.2 | [`ALIENTEK_iMX6ULL_Linux_Driver_Development_Guide_V1.5.2.pdf`](ALIENTEK_iMX6ULL_Linux_Driver_Development_Guide_V1.5.2.pdf) | [GitHub archive](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf) | U-Boot, kernel build, modules, DeviceTree, platform driver |
| `SRC-IMX6ULL-APP` | ALIENTEK I.MX6U Embedded Linux C Application Programming Guide V1.1 | [`ALIENTEK_iMX6ULL_Linux_C_Application_Programming_Guide_V1.1.pdf`](ALIENTEK_iMX6ULL_Linux_C_Application_Programming_Guide_V1.1.pdf) | [GitHub archive](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf) | syscall, file I/O, process, virtual address space |
| `SRC-IMX6ULL-TFTP-NFS` | ALIENTEK I.MX6U TFTP & NFS Setup Guide V1.3.1 | [`ALIENTEK_iMX6ULL_TFTP_NFS_Setup_Guide_V1.3.1.pdf`](ALIENTEK_iMX6ULL_TFTP_NFS_Setup_Guide_V1.3.1.pdf) | [GitHub archive](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%BD%91%E7%BB%9C%E7%8E%AF%E5%A2%83TFTP%26NFS%E6%90%AD%E5%BB%BA%E6%89%8B%E5%86%8CV1.3.1.pdf) | board/VM network, TFTP, NFS |
| `SRC-F407-SCH` | ALIENTEK Explorer STM32F4 V2.2 Schematic | [`Explorer_STM32F4_V2.2_SCH.pdf`](Explorer_STM32F4_V2.2_SCH.pdf) | local course copy | STM32F407ZET6 board facts |
| `SRC-F407-HAL` | ALIENTEK STM32F4 Development Guide - HAL V1.2 | [`ALIENTEK_STM32F4_HAL_Development_Guide_V1.2.pdf`](ALIENTEK_STM32F4_HAL_Development_Guide_V1.2.pdf) | [ALIENTEK download center](http://www.openedv.com/docs/index.html) | optional cross-check of board/peripheral use |
| `SRC-F407-REG` | ALIENTEK STM32F4 Development Guide - Register V1.2 | [`ALIENTEK_STM32F4_Register_Development_Guide_V1.2.pdf`](ALIENTEK_STM32F4_Register_Development_Guide_V1.2.pdf) | [ALIENTEK download center](http://www.openedv.com/docs/index.html) | optional register-level cross-check |
| `SRC-ST-RM0090` | ST RM0090 STM32F407 Reference Manual | - | [ST direct PDF](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) | RCC/GPIO/USART/debug register semantics |
| `SRC-ST-PM0214` | ST Cortex-M4 Programming Manual PM0214 | - | [ST direct PDF](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf) | exception/debug/core architecture |
| `SRC-LINUX-KBUILD-MODULES` | Linux Kernel: Building External Modules | - | [Kernel docs](https://docs.kernel.org/kbuild/modules.html) | out-of-tree modules, Kbuild |
| `SRC-LINUX-KCONFIG` | Linux Kernel: Kconfig configuration targets/editors | - | [Kernel docs](https://docs.kernel.org/kbuild/kconfig.html) | menuconfig/.config |
| `SRC-LINUX-DT` | Linux and the Devicetree | - | [Kernel Chinese docs](https://docs.kernel.org/translations/zh_CN/devicetree/usage-model.html) | FDT, device population, platform_device |
| `SRC-ZEPHYR-BOARD` | Zephyr Board Porting Guide | - | [Zephyr docs](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html) | out-of-tree board support |
| `SRC-ZEPHYR-DT` | Zephyr Devicetree Guide | - | [Zephyr docs](https://docs.zephyrproject.org/latest/build/dts/index.html) | DTS/build-time model |
| `SRC-ZEPHYR-GPIO` | Zephyr GPIO API/Devicetree | - | [Zephyr docs](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html) | LED/KEY GPIO |
| `SRC-ZEPHYR-DEBUG` | Zephyr west build/flash/debug | - | [Zephyr docs](https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html) | `west debug`, runners |
| `SRC-ZEPHYR-THREADS` | Zephyr Threads/Scheduling | - | [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) / [Scheduling](https://docs.zephyrproject.org/latest/kernel/services/scheduling/index.html) | RTOS thread/scheduler model |
| `SRC-ZEPHYR-THREAD-ANALYZER` | Zephyr Thread Analyzer | - | [Zephyr docs](https://docs.zephyrproject.org/latest/services/debugging/thread-analyzer.html) | runtime stack/thread budget |

## Local manual naming rule

If your local PDFs still have Chinese filenames, copy or rename them to the ASCII aliases above under `references/`. The tutorials always give both the local relative link and the public link; the public link remains usable even if the local PDF has not been copied yet.
