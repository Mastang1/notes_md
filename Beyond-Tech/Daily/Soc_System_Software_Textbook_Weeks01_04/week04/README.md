# Part IV - Enter the Kernel and the RTOS Runtime

> Week 4 的核心问题：**Kernel/RTOS 运行时到底怎样构建、装载、调度和失败；我如何用工具看到这些机制？**

```mermaid
flowchart LR
 K[Kconfig/Kbuild] --> M[Kernel Module]
 M --> S[Symbols/ABI]
 S --> O[Oops Debug]
 O --> Z[Zephyr Scheduling]
 Z --> R[Resource Budget]
 R --> I[Cross-OS Integration]
```

## Reading order

- [Chapter 22 - How a Linux Kernel Is Built](ch22_kernel_build_kconfig_kbuild.md) - Kconfig/Kbuild/产物
- [Chapter 23 - Your First Kernel Module](ch23_first_kernel_module.md) - build/load/unload
- [Chapter 24 - How a Module Joins a Running Kernel](ch24_module_symbols_vermagic.md) - symbol/dependency/vermagic
- [Chapter 25 - When the Kernel Crashes](ch25_kernel_oops_debug.md) - Oops/source 定位
- [Chapter 26 - Zephyr Scheduling as a Running System](ch26_zephyr_scheduling.md) - thread/priority/wakeup
- [Chapter 27 - RTOS Memory Is a Budget](ch27_zephyr_memory_thread_analyzer.md) - stack/heap/analyzer
- [Chapter 28 - Integration](ch28_week4_integration.md) - Linux vs Zephyr runtime model

## Week 4 final capability

Linux 侧能够自己构建/装载 Module、解释符号与 ABI、完成一次 Oops 到源码的定位；Zephyr 侧能够用运行实验解释调度/同步并量化 thread stack 预算。

## Manuals and primary references

- [ALIENTEK Linux Driver Guide V1.5.2](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- [Linux Kbuild](https://docs.kernel.org/kbuild/index.html)
- [Linux External Modules](https://docs.kernel.org/kbuild/modules.html)
- [Linux Bug Hunting](https://docs.kernel.org/admin-guide/bug-hunting.html)
- [Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)
- [Zephyr Thread Analyzer](https://docs.zephyrproject.org/latest/services/debugging/thread-analyzer.html)
- [Unified source index](../common/source_index.md)

## Manual reading map

| Manual | Local expected filename | Online | This Part uses |
|---|---|---|---|
| I.MX6U Linux Driver Guide V1.5.2 | `ALIENTEK_iMX6ULL_Linux_Driver_Guide_V1.5.2.pdf` | [Open PDF page](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf) | Ch22-25: Kernel build/module/debug；具体章节按 PDF 目录检索 `模块/insmod/Kconfig` |
| Linux Kernel Kbuild | n/a | [Kbuild](https://docs.kernel.org/kbuild/index.html) | Ch22 |
| External Modules | n/a | [Modules](https://docs.kernel.org/kbuild/modules.html) | Ch23-24 |
| Kernel Bug Hunting | n/a | [Bug hunting](https://docs.kernel.org/admin-guide/bug-hunting.html) | Ch25 |
| Zephyr Threads/Analyzer | n/a | [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) | Ch26-27 |

