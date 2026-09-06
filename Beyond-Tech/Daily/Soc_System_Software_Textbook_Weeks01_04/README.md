# SoC System Software Textbook - Weeks 1-4

> 这是 20 周总计划的前四周教材化重写版。**总体学习目标和日程顺序不变；写法改为技术书籍式的连续章节。** 每章都承接前章的问题，以 explained example -> guided lab -> independent challenge 递进。

## How to read

1. 按 Chapter 顺序读，不建议跳章。
2. 每章核心学习块约 2h；“Independent Challenge”做不完可以延续到周末，但不要提前抢下一章。
3. 所有 i.MX6ULL 板型参数以你自己的 BSP/手册为准；STM32F4 板级事实以用户上传 Explorer V2.2 原理图为准。
4. 每章末尾的 `References and manuals` 都提供在线链接和/或本地预期路径。

## Parts

- [Part I - Establish the Development Loop](week01/README.md) - Chapters 1-7
- [Part II - Own the BSP and the Board](week02/README.md) - Chapters 8-14
- [Part III - From User Space to the Kernel Boundary](week03/README.md) - Chapters 15-21
- [Part IV - Enter the Kernel and RTOS Runtime](week04/README.md) - Chapters 22-28

## Complete chapter index

- [Chapter 1 - Build a Reproducible Embedded Linux Development Host](week01/ch01_linux_host.md)
- [Chapter 2 - Cross Compilation: From C Source to ARM ELF](week01/ch02_cross_compilation_elf.md)
- [Chapter 3 - First Contact with i.MX6ULL: Serial Console, U-Boot and Network](week01/ch03_imx6ull_console_network.md)
- [Chapter 4 - Shorten the Development Cycle: TFTP and NFS](week01/ch04_tftp_nfs_loop.md)
- [Chapter 5 - Understand the Zephyr Build World: west, Kconfig and Devicetree](week01/ch05_zephyr_build_world.md)
- [Chapter 6 - Hardware Before Software: Audit the STM32F407 Explorer Board](week01/ch06_f407_hardware_audit.md)
- [Chapter 7 - Integration: Reproduce the Entire Week from a Clean State](week01/ch07_week1_integration.md)
- [Chapter 8 - What Exactly Is a BSP? Map Source Code to Boot Artifacts](week02/ch08_bsp_artifact_map.md)
- [Chapter 9 - Build the Boot Chain Yourself: U-Boot, Kernel and DTB](week02/ch09_build_uboot_kernel_dtb.md)
- [Chapter 10 - Boot Without Flashing: Load Kernel and DTB into RAM with TFTP](week02/ch10_tftp_ram_boot.md)
- [Chapter 11 - Debug Across Architectures: gdb-multiarch and gdbserver](week02/ch11_remote_gdb.md)
- [Chapter 12 - Build a Zephyr Board from Scratch: Out-of-Tree Board Architecture](week02/ch12_zephyr_out_of_tree_board.md)
- [Chapter 13 - Bring the STM32F407 Board Alive: Clock, USART1 Console and Flashing](week02/ch13_f407_clock_console_flash.md)
- [Chapter 14 - Integration: Clean Build and Linux/Zephyr Hardware Description Comparison](week02/ch14_week2_integration.md)
- [Chapter 15 - Process Is Not Program: fork, exec, wait and syscall](week03/ch15_process_fork_exec_syscall.md)
- [Chapter 16 - How Linux Loads an Executable: ELF, Loader and Dynamic Linker](week03/ch16_elf_loader_dynamic_linker.md)
- [Chapter 17 - Leave the MCU Physical-Memory Model: Virtual Address, Page and Mapping](week03/ch17_virtual_memory.md)
- [Chapter 18 - Why Everything Looks Like a File: fd, struct file and UAPI](week03/ch18_file_descriptor_uapi.md)
- [Chapter 19 - Describe STM32F407 Devices in Zephyr: LED, KEY and Devicetree](week03/ch19_zephyr_led_key_devicetree.md)
- [Chapter 20 - Stop Debugging with printf: west debug, GDB and Target State](week03/ch20_zephyr_west_debug.md)
- [Chapter 21 - Integration: Reconstruct User Space and Zephyr Device Models from Memory](week03/ch21_week3_integration.md)
- [Chapter 22 - How a Linux Kernel Is Built: Kconfig, Kbuild and Artifacts](week04/ch22_kernel_build_kconfig_kbuild.md)
- [Chapter 23 - Your First Kernel Module: Build, Load, Execute and Unload](week04/ch23_first_kernel_module.md)
- [Chapter 24 - How a Module Joins a Running Kernel: Symbols, Dependencies and vermagic](week04/ch24_module_symbols_vermagic.md)
- [Chapter 25 - When the Kernel Crashes: Oops, Address, Symbol and Source Line](week04/ch25_kernel_oops_debug.md)
- [Chapter 26 - Zephyr Scheduling as a Running System: Thread, Priority and Wakeup](week04/ch26_zephyr_scheduling.md)
- [Chapter 27 - RTOS Memory Is a Budget: Stack, Heap and Thread Analyzer](week04/ch27_zephyr_memory_thread_analyzer.md)
- [Chapter 28 - Integration: Linux Process/Thread vs Zephyr Thread/Context](week04/ch28_week4_integration.md)

## Manuals and references

- [Unified source index](common/source_index.md)
- [Reference directory](references/README.md)
- [Local STM32F4 Explorer V2.2 schematic](references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf)
- [ALIENTEK i.MX6ULL archived manuals repository](https://github.com/alientek-openedv/imx6ull-document)
- [Linux Kernel Documentation](https://docs.kernel.org/)
- [Zephyr Documentation](https://docs.zephyrproject.org/latest/)

## Four-week capability chain

```mermaid
flowchart LR
  W1[Week 1: Development Loop] --> W2[Week 2: BSP and Board]
  W2 --> W3[Week 3: User Space to Kernel Boundary]
  W3 --> W4[Week 4: Kernel Module and RTOS Runtime]
  W4 --> W5[Week 5: DeviceTree and Driver Model]
```
