# Chapter 2 - Cross Compilation: From C Source to ARM ELF

> Week 1 / Day 2 - 从你熟悉的 linker/startup 迁移到 Linux ELF。

[← Part README](README.md) · [← Previous](ch01_linux_host.md) · [Next →](ch03_imx6ull_console_network.md)

## 2.1 从一个你已经熟悉的问题开始：MCU 工程为什么需要 linker script

你已经改过 startup.s 和 link file，所以不要从“什么叫编译器”重学。真正需要迁移的是：**裸机/RTOS 固件和 Linux ELF 的生成链相同，但运行时契约不同。**

```mermaid
flowchart LR
    C[hello.c] --> PP[Preprocessor]
    PP --> CC[Compiler]
    CC --> AS[Assembler]
    AS --> OBJ[hello.o]
    OBJ --> LD[Linker]
    LD --> ELF[ELF executable]
```

GCC 命令本身更像一个 driver：它根据选项调度预处理器、编译器、assembler、linker。知道这一点后，很多“gcc 报错”其实可以先判断发生在链路的哪一段。

## 2.2 Host、Build、Target：为什么一台 x86 能制造 ARM 二进制

今天先使用最实用的两角色：

- Host：你当前执行编译器的 x86 Ubuntu；
- Target：最终运行程序的 ARM i.MX6ULL。

交叉工具链名字 `arm-linux-gnueabihf-` 本身就在表达目标 ABI：ARM、Linux、GNU EABI、hard-float。

```bash
which gcc
which arm-linux-gnueabihf-gcc || true
gcc -dumpmachine
arm-linux-gnueabihf-gcc -dumpmachine
```

如果你的正点原子 BSP 附带的是另一条工具链，以 BSP 实际工具链为准，不要为了命令长得一样强行换版本。

## 2.3 Toolchain 不只是 gcc：把它看成一组针对同一 ABI 的工具

```text
arm-linux-gnueabihf-gcc      编译/驱动
arm-linux-gnueabihf-ld       链接
arm-linux-gnueabihf-as       汇编
arm-linux-gnueabihf-objdump  反汇编/对象查看
arm-linux-gnueabihf-readelf  ELF 结构
arm-linux-gnueabihf-nm       符号
arm-linux-gnueabihf-strip    去符号
```

再加一个经常被忽略的概念：**sysroot**。Linux 应用不仅依赖 CPU 指令集，还依赖目标系统的 libc、头文件和动态加载器。sysroot 就是在 Host 上提供“Target 根文件系统的一小部分视图”。

## 2.4 Worked Example：让每一步产物都显形

建立 `hello.c`：

```c
#include <stdio.h>
int global_value = 7;
static int static_value;

int add(int a, int b) { return a + b; }

int main(void)
{
    static_value = add(global_value, 5);
    printf("value=%d\n", static_value);
    return 0;
}
```

逐步生成：

```bash
gcc -E hello.c -o hello.i
gcc -S hello.i -o hello.s
gcc -c hello.s -o hello.o
gcc hello.o -o hello_x86
```

观察每个阶段：

```bash
file hello.o hello_x86
nm -n hello.o | head
objdump -d hello.o | less
```

你应该把错误分层：

```text
语法/类型错误       -> compiler
undefined reference -> linker
Exec format error   -> loader/architecture mismatch
```

## 2.5 ELF Header：程序加载前，内核首先需要知道“你是谁”

```bash
readelf -h hello_x86
```

重点只看：

- `Class`：32/64 bit；
- `Data`：大小端；
- `Machine`：目标架构；
- `Entry point address`；
- Program/Section Header 数量。

接着生成 ARM 版本：

```bash
arm-linux-gnueabihf-gcc -g -O0 hello.c -o hello_arm
file hello_arm
readelf -h hello_arm
```

同一份 C 代码，ELF 容器格式仍然成立，但 Machine、指令、动态解释器发生改变。

## 2.6 Section 与 Segment：这是本章最重要的概念区分

你在 MCU link script 里熟悉 `.text/.data/.bss`，它们属于 **link-time 组织单位 Section**。

Linux loader 真正按照 **Program Header / Segment** 建立进程地址空间：

```bash
readelf -S hello_arm
readelf -l hello_arm
```

费曼解释：

> Section 像仓库里按物品类别分箱，方便编译器/linker 管理；Segment 像运输时重新按“这一车要只读可执行、那一车可读写”装车，方便 loader 映射页面。

所以多个 section 可以被装进一个 LOAD segment。这正是从 MCU 链接视角走向 Linux loader 视角的桥。

## 2.7 Symbol、Relocation 与动态依赖：先建立位置，不在今天深挖

```bash
nm -n hello_arm | grep ' main\| add'
readelf -s hello_arm | less
readelf -r hello_arm | head -40
readelf -d hello_arm | head -40
```

今天只需要知道：

- symbol 给函数/对象一个可被链接和调试引用的名字；
- relocation 记录“这个位置最终要填什么地址/偏移”；
- dynamic section 描述运行时动态链接所需信息。

Week 3 会再从 loader/动态链接器角度回来。

## 2.8 Guided Lab：x86 与 ARM ELF 做一张差异表

至少比较：

| 项目 | x86 ELF | ARM ELF |
|---|---|---|
| Machine | | |
| Class | | |
| Entry | | |
| Interpreter | | |
| main symbol | | |
| LOAD segments | | |

使用 `readelf -l` 找 `Requesting program interpreter`。

## 2.9 故障实验：在 x86 Host 直接执行 ARM ELF

```bash
./hello_arm
```

典型结果是 `Exec format error`。不要把它理解成“Linux 不认 ELF”，而是内核 ELF loader 识别到 ELF 以后发现 `e_machine` 与当前 CPU 不匹配，不能建立可执行映像。

如果你的 Host 安装了 qemu-user 并配置了 binfmt_misc，可能会被透明模拟而执行成功。那反而是一个好问题：去查 `/proc/sys/fs/binfmt_misc/`，确认是谁替你做了架构翻译。

## 2.10 Independent Challenge：不用 IDE，回答“main 不是程序第一条指令”

用：

```bash
readelf -h hello_arm | grep Entry
arm-linux-gnueabihf-objdump -d hello_arm | less
```

找到 Entry 附近代码，再找 `main`。写 5 句话解释：Linux ELF 进入用户态后为什么通常还有 C runtime/startup 工作才到 `main()`。

这和你熟悉的 MCU `Reset_Handler -> SystemInit -> __main/main` 有强烈同构关系，只是 Linux 进程的启动上下文由内核 loader 与 C runtime 共同准备。

## 2.11 本章小结

本章结束时，你的心智模型应该变成：

```text
C source
  -> architecture-specific object
  -> linker produces ELF
  -> ELF contains link-time Sections + load-time Segments
  -> target kernel/loader checks architecture and maps it
```

## 2.12 下一章：ARM ELF 已经有了，但 Target 还只是一个黑盒

下一章不急着 SCP 程序。先通过串口从上电开始看：BootROM 之后谁接管、为什么先看到 U-Boot、Linux console 又是什么。把开发板从“能启动的盒子”变成可观察系统。

## References and manuals

### ALIENTEK Linux C Application Guide V1.1
- Local expected path: `../references/ALIENTEK_iMX6ULL_Linux_C_Application_Guide_V1.1.pdf`
- Online: [ALIENTEK Linux C Application Guide V1.1](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- 本章阅读定位：查找“交叉编译”“GCC”“ELF/可执行文件”相关章节；本章用于工具操作，ELF 精确模型以 readelf/man-pages 实验为主。

### ELF manual page

- Online: [ELF manual page](https://man7.org/linux/man-pages/man5/elf.5.html)
- 本章阅读定位：重点看 ELF header、program header、section header 的职责。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch01_linux_host.md) · [Next →](ch03_imx6ull_console_network.md)
