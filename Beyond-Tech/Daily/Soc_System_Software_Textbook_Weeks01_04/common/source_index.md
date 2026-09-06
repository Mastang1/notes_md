# Source and Manual Index

本文件是 Week 1-4 教材的统一资料索引。**教程正文不会用“参考一下某手册”这种模糊写法**；每章会说明读哪本、定位什么章节/关键词、为什么读。

> 正点原子 GitHub 仓库是历史光盘资料存档；公开仓库 README 也提示新资料应到其资料中心获取。若本地手册版本与在线版不同，以你手头开发板配套版本为先。

## ALIENTEK i.MX6ULL manuals

### SRC-ALIENTEK-DRV
- Manual: `【正点原子】I.MX6U嵌入式Linux驱动开发指南V1.5.2.pdf`
- Expected local path: `../references/ALIENTEK_iMX6ULL_Linux_Driver_Guide_V1.5.2.pdf`
- Online: [【正点原子】I.MX6U嵌入式Linux驱动开发指南V1.5.2.pdf](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)

### SRC-ALIENTEK-APP
- Manual: `【正点原子】I.MX6U嵌入式Linux C应用编程指南V1.1.pdf`
- Expected local path: `../references/ALIENTEK_iMX6ULL_Linux_C_Application_Guide_V1.1.pdf`
- Online: [【正点原子】I.MX6U嵌入式Linux C应用编程指南V1.1.pdf](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)

### SRC-ALIENTEK-TFTP
- Manual: `【正点原子】I.MX6U网络环境TFTP&NFS搭建手册V1.3.1.pdf`
- Expected local path: `../references/ALIENTEK_iMX6ULL_TFTP_NFS_Guide_V1.3.1.pdf`
- Online: [【正点原子】I.MX6U网络环境TFTP&NFS搭建手册V1.3.1.pdf](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%BD%91%E7%BB%9C%E7%8E%AF%E5%A2%83TFTP%26NFS%E6%90%AD%E5%BB%BA%E6%89%8B%E5%86%8CV1.3.1.pdf)

### SRC-ALIENTEK-VM
- Manual: `【正点原子】I.MX6U虚拟机使用参考手册V1.2.pdf`
- Expected local path: `../references/ALIENTEK_iMX6ULL_VM_Guide_V1.2.pdf`
- Online: [【正点原子】I.MX6U虚拟机使用参考手册V1.2.pdf](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E8%99%9A%E6%8B%9F%E6%9C%BA%E4%BD%BF%E7%94%A8%E5%8F%82%E8%80%83%E6%89%8B%E5%86%8CV1.2.pdf)

### SRC-ALIENTEK-QUICK
- Manual: `【正点原子】I.MX6U用户快速体验V1.8.pdf`
- Expected local path: `../references/ALIENTEK_iMX6ULL_Quick_Start_V1.8.pdf`
- Online: [【正点原子】I.MX6U用户快速体验V1.8.pdf](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%94%A8%E6%88%B7%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8CV1.8.pdf)


## Known manual chapter anchors used by Weeks 1-4

这些章节号来自正点原子公开在线文档/公开版本目录，用于快速定位。**你截图中的 PDF 版本若页码不同，以章节标题/小节号为准，不强行套页码。**

### I.MX6U Linux Driver Development Guide
- Chapter 4: 开发环境搭建。公开在线文档明确可见该章标题。
- Chapter 33: U-Boot 移植。
- Chapter 43: Linux 设备树。
- Chapter 55: 设备树下的 platform 驱动编写（用于后续 Week 5）。

### I.MX6U Quick Start
- Chapter 4: 交叉编译。
- 4.1: 安装通用 ARM 交叉编译工具链。
- 4.3: 编译出厂源码 U-Boot。
- 4.4: 编译出厂源码内核及模块。
- 4.6: 编译一个简单的 C 文件。

### I.MX6U Linux C Application Guide
公开后续版本目录可用于章节定位：
- Chapter 2: 文件 I/O 基础（open/read/write/close）。
- Chapter 9: 进程。
- 9.3: 进程的内存布局。
- 9.4: 进程的虚拟地址空间。
- 9.5: fork() 创建子进程。
- 9.10: wait/waitpid 等监视子进程。
- Chapter 11: 线程。
- Chapter 12: 线程同步。

## STM32F4 board source

### SRC-F407-SCH
- Manual: `Explorer STM32F4 V2.2 Schematic`
- Local: [ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf](../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf)
- Pages used in this course: p.1 Ethernet/audio; p.2 MCU/core/clock/JTAG/UART nets; p.3 W25Q128/LED/KEY/CAN/RS485; p.4 CH340G USB-UART/power; p.5 board placement.

## Official Linux references

- Linux Kernel Kbuild: https://docs.kernel.org/kbuild/index.html
- External modules: https://docs.kernel.org/kbuild/modules.html
- DeviceTree usage model: https://docs.kernel.org/devicetree/usage-model.html
- Driver model: https://docs.kernel.org/driver-api/driver-model/index.html
- Platform devices/drivers: https://docs.kernel.org/driver-api/driver-model/platform.html
- Kernel debugging/bug hunting: https://docs.kernel.org/admin-guide/bug-hunting.html
- Linux man-pages: https://man7.org/linux/man-pages/
- GDB Remote Debugging: https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html

## Official Zephyr references

- Getting Started: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
- west: https://docs.zephyrproject.org/latest/develop/west/index.html
- Board porting: https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html
- Devicetree: https://docs.zephyrproject.org/latest/build/dts/index.html
- Devicetree vs Kconfig: https://docs.zephyrproject.org/latest/build/dts/dt-vs-kconfig.html
- Kconfig: https://docs.zephyrproject.org/latest/build/kconfig/index.html
- Debugging: https://docs.zephyrproject.org/latest/develop/debug/index.html
- Threads: https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
- Thread analyzer: https://docs.zephyrproject.org/latest/services/debugging/thread-analyzer.html

## Local-manual naming rule

为避免 Windows/ZIP 中文文件名乱码，本课程只使用 ASCII 文件名。你可以把从正点原子下载的 PDF 重命名为上面 `Expected local path` 中的文件名。教程内同时保留在线链接，因此没有本地 PDF 时也能直接打开公开存档。