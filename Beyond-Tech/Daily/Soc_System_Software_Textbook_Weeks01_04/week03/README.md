# Part III - From User Space to the Kernel Boundary

> Week 3 的核心问题：**Linux 用户程序到底怎样被创建、加载、映射内存并通过 fd/syscall 进入内核；Zephyr 板级硬件又怎样变成可调试 device？**

```mermaid
flowchart LR
 P[Process] --> E[ELF Loader]
 E --> V[Virtual Memory]
 V --> F[FD/UAPI]
 F --> Z[Zephyr DT Devices]
 Z --> G[GDB/Debug]
 G --> R[Reconstruct Models]
```

## Reading order

- [Chapter 15 - Process Is Not Program](ch15_process_fork_exec_syscall.md) - fork/exec/wait/syscall
- [Chapter 16 - How Linux Loads an Executable](ch16_elf_loader_dynamic_linker.md) - ELF loader/dynamic linker
- [Chapter 17 - Leave the MCU Physical-Memory Model](ch17_virtual_memory.md) - VA/page/MMU
- [Chapter 18 - Why Everything Looks Like a File](ch18_file_descriptor_uapi.md) - fd/struct file/UAPI
- [Chapter 19 - Describe STM32F407 Devices in Zephyr](ch19_zephyr_led_key_devicetree.md) - LED/KEY/DT
- [Chapter 20 - Stop Debugging with printf](ch20_zephyr_west_debug.md) - west debug/GDB
- [Chapter 21 - Integration](ch21_week3_integration.md) - 用户态与 Zephyr object 模型

## Week 3 final capability

能够用 strace/GDB/readelf/maps 从运行现象解释 Linux 用户程序的系统行为，并建立 fd -> struct file -> operations 的 Driver 入口模型；Zephyr F407 完成 LED/KEY hardware description 与 on-target debug。

## Manuals and primary references

- [ALIENTEK Linux C Application Guide V1.1](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- [Linux man-pages](https://man7.org/linux/man-pages/)
- [Local STM32F4 Explorer schematic](../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf)
- [Zephyr Devicetree](https://docs.zephyrproject.org/latest/build/dts/index.html)
- [Zephyr Debugging](https://docs.zephyrproject.org/latest/develop/debug/index.html)
- [Unified source index](../common/source_index.md)

## Manual reading map

| Manual | Local expected filename | Online | This Part uses |
|---|---|---|---|
| I.MX6U Linux C Application Guide V1.1 | `ALIENTEK_iMX6ULL_Linux_C_Application_Guide_V1.1.pdf` | [Open PDF page](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf) | Ch15/17: Chapter 9 process; 9.3 memory layout; 9.4 VA; 9.5 fork; 9.10 wait. Ch18: Chapter 2 file I/O. |
| Explorer STM32F4 V2.2 Schematic | packaged | [Local PDF](../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf) | Ch19: PF9/PF10 LED, PE2-4 KEY |
| Linux man-pages | n/a | [man7 index](https://man7.org/linux/man-pages/) | Ch15-18 precise syscall semantics |

