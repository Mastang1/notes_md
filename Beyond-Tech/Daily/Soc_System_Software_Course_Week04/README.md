# Week 04 - Linux Kernel/Kbuild/Modules + Zephyr Kernel Usage

## Week goal

- Independently build/load/debug a minimal Linux kernel module and understand Kconfig/Kbuild, symbols/vermagic, and first Oops decoding.
- Move Zephyr knowledge from “API calls” to scheduling/synchronization/resource-budget reasoning.

## Daily tutorials

| Day | Tutorial | Main output | Manual / official reading |
|---|---|---|---|
| D1 | [Kernel Kconfig/Kbuild](linux/W04D01_Kernel_Kconfig_Kbuild.md) | CONFIG→object→artifact trace | Driver Guide Ch.35 + Kernel Kconfig |
| D2 | [Hello Kernel Module](linux/W04D02_Hello_Kernel_Module.md) | `.ko` + module_param | Driver Guide Ch.40 + Kernel external modules |
| D3 | [Module Symbols/Vermagic](linux/W04D03_Module_Symbols_and_Vermagic.md) | mismatch diagnosis | Kernel modules + Module.symvers |
| D4 | [Minimal Kernel Oops](linux/W04D04_Minimal_Kernel_Oops.md) | warning/Oops decode | exact vmlinux/ko + GDB/binutils |
| D5 | [Zephyr Threads/Scheduling/Sync](zephyr/W04D05_Threads_Scheduling_and_Sync.md) | 3-thread timing proof | Zephyr Threads/Scheduling/Sem/MsgQ |
| D6 | [Thread Analyzer and Stack Budget](zephyr/W04D06_Thread_Analyzer_and_Stack_Budget.md) | stack budget table | Zephyr Thread Analyzer |
| D7 | [Linux vs Zephyr Runtime Gate](linux/W04D07_Linux_vs_Zephyr_Runtime.md) | engineering comparison | targeted re-read |

## Manuals and direct links

- [ALIENTEK i.MX6ULL Linux Driver Guide V1.5.2 - local](references/ALIENTEK_iMX6ULL_Linux_Driver_Development_Guide_V1.5.2.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- [ALIENTEK i.MX6ULL Linux C Application Guide V1.1 - local](references/ALIENTEK_iMX6ULL_Linux_C_Application_Programming_Guide_V1.1.pdf) / [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- [Linux Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- [Linux Kconfig](https://docs.kernel.org/kbuild/kconfig.html)
- [Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)
- [Zephyr Scheduling](https://docs.zephyrproject.org/latest/kernel/services/scheduling/index.html)
- [Zephyr Thread Analyzer](https://docs.zephyrproject.org/latest/services/debugging/thread-analyzer.html)
- [Explorer STM32F4 schematic](references/Explorer_STM32F4_V2.2_SCH.pdf)
- [Source index](references/source_index.md)
- [Local manual filename mapping](references/README.md)
