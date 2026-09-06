# Chapter 2 - 从 `hello.c` 到 ARM ELF：真正理解交叉编译

## 2.1 本章任务

今天不碰开发板。先把一个最重要的问题讲透：

> **为什么一台 x86_64 Ubuntu 能编译出只能在 ARM Cortex-A7 上运行的程序？**

完成后你必须能拿两个 ELF 文件，仅靠 `file/readelf/objdump` 判断：
- 哪个给 Host；
- 哪个给 i.MX6ULL；
- 入口地址是什么；
- 哪些是 Section；
- 哪些是 Segment；
- 是否动态链接；
- 需要什么 interpreter。

### 本章产物
```text
~/work/linux/apps/week1_elf/
├── hello.c
├── hello_x86
├── hello_arm
├── hello_arm_static
├── hello.i
├── hello.s
├── hello.o
└── analysis.txt
```

---

## 2.2 先把“编译器”这个词拆开

一个 C 程序到 ELF 至少经历：

```mermaid
flowchart LR
    C["hello.c"]
    PP["Preprocessor"]
    CC["Compiler"]
    AS["Assembler"]
    OBJ["hello.o"]
    LD["Linker"]
    ELF["ELF executable"]

    C --> PP --> CC --> AS --> OBJ --> LD --> ELF
```

GCC 命令通常扮演“driver”，负责按选项调度预处理器、编译器、汇编器和 linker。

### 和 MCU 的知识迁移

STM32：

```text
startup.s + main.c + HAL
         ↓
      compiler
         ↓
      .o files
         ↓
 linker script (.ld)
         ↓
      ELF/AXF
         ↓
 bin/hex -> Flash
```

Linux 用户态：

```text
main.c + libc
     ↓
compiler
     ↓
.o
     ↓
linker
     ↓
ELF executable
     ↓
Linux ELF loader
```

核心没有变：**Object + Link**。差别主要是最终执行环境和 ABI。

---

## 2.3 安装 Week 1 使用的 ARM Linux 工具链

Ubuntu Host：

```bash
sudo apt update
sudo apt install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf \
    gdb-multiarch
```

验证：

```bash
which arm-linux-gnueabihf-gcc
arm-linux-gnueabihf-gcc --version
arm-linux-gnueabihf-gcc -dumpmachine
```

预期重点：

```text
arm-linux-gnueabihf
```

### 为什么不在 Week 1 直接照抄正点原子 Linaro 4.9.4

《I.MX6U Linux 驱动开发指南 V1.5.2》4.3 是基于老 BSP/Ubuntu 环境编写的，并明确使用 Linaro 4.9.4。这个工具链对于后面复现其 U-Boot 2016 / Linux 4.1.15 很重要。

但你现在的 Host 是 **Ubuntu 24.04**。Week 1 的目标只是理解“Linux ARM 用户态交叉编译”，因此先使用 Ubuntu 当前包管理器提供的 `arm-linux-gnueabihf` 工具链，避免把“学习 ELF”和“兼容老 BSP”两个问题混在一起。

后面需要编译旧 BSP 时，我们会把 Linaro 4.9.4 放进独立目录，不覆盖系统 toolchain。

---

## 2.4 Worked Example：同一个源码生成两种架构的 ELF

进入目录：

```bash
mkdir -p ~/work/linux/apps/week1_elf
cd ~/work/linux/apps/week1_elf
```

建立 `hello.c`：

```c
#include <stdio.h>

int main(void)
{
    printf("hello from week1\n");
    return 0;
}
```

### 2.4.1 编译 Host 版本

```bash
gcc -Wall -Wextra -O0 -g hello.c -o hello_x86
```

逐项解释：

- `-Wall -Wextra`：开启常见警告；
- `-O0`：关闭优化，方便学习汇编和调试；
- `-g`：保留 DWARF 调试信息；
- `hello.c`：输入；
- `-o hello_x86`：指定输出。

运行：

```bash
./hello_x86
```

预期：

```text
hello from week1
```

确认架构：

```bash
file hello_x86
```

典型输出会包含：

```text
ELF 64-bit LSB pie executable, x86-64
```

### 2.4.2 编译 Target 版本

```bash
arm-linux-gnueabihf-gcc -Wall -Wextra -O0 -g hello.c -o hello_arm
```

查看：

```bash
file hello_arm
```

应包含：

```text
ELF 32-bit LSB ... ARM
```

此时不要运行它。先预测：

> 在 x86 Host 上直接执行 `./hello_arm` 会发生什么？

然后执行：

```bash
./hello_arm
```

典型结果：

```text
bash: ./hello_arm: cannot execute binary file: Exec format error
```

这不是权限问题，也不是文件坏了。

---

## 2.5 把 GCC 流程人为拆开

### 2.5.1 预处理

```bash
arm-linux-gnueabihf-gcc -E hello.c -o hello.i
```

`-E`：只预处理。

观察：

```bash
wc -l hello.c hello.i
head -40 hello.i
```

### 2.5.2 编译到汇编

```bash
arm-linux-gnueabihf-gcc -S -O0 hello.c -o hello.s
```

`-S`：到汇编停止，不运行 assembler。

查看：

```bash
sed -n '1,120p' hello.s
```

### 2.5.3 汇编成 Object

```bash
arm-linux-gnueabihf-gcc -c -O0 -g hello.c -o hello.o
```

`-c`：编译/汇编，但不 link。

确认：

```bash
file hello.o
readelf -h hello.o
```

`.o` 也是 ELF，但它通常是 **relocatable object**。

### 2.5.4 Link

```bash
arm-linux-gnueabihf-gcc hello.o -o hello_arm
```

这一步把你的 object、启动对象和 libc 等组合成最终可执行 ELF。

---

## 2.6 ELF Header：先学会读“身份证”

执行：

```bash
readelf -h hello_x86
readelf -h hello_arm
```

重点只看：

```text
Class:
Data:
Type:
Machine:
Entry point address:
```

> **Entry Point 不一定是 `main()`。**

Linux 通常先进入 C runtime startup，再由 runtime 调用 `main()`。这与 MCU 中 `Reset_Handler -> SystemInit -> main` 的思路非常接近。

---

## 2.7 Section 与 Segment：Day 2 最重要的概念

### 2.7.1 Section 是“链接器视角”

```bash
readelf -S hello_arm
```

会看到：

```text
.text
.rodata
.data
.bss
.symtab
.strtab
```

你熟悉的 MCU linker script 也是在组织这些 Section。

### 2.7.2 Segment 是“Loader 视角”

```bash
readelf -l hello_arm
```

关注：

```text
LOAD
INTERP
DYNAMIC
GNU_STACK
```

费曼版：

> **Section 是做程序的人整理材料的抽屉；Segment 是 Linux Loader 真正搬进虚拟地址空间的箱子。**

### 2.7.3 自己证明 Section -> Segment

```bash
readelf -lW hello_arm
```

在输出底部找：

```text
Section to Segment mapping
```

亲眼看 `.text/.rodata/.data` 被组合进哪些 LOAD segment。

---

## 2.8 动态链接器为什么重要

查看：

```bash
readelf -l hello_arm | grep -A1 INTERP
```

可能看到：

```text
Requesting program interpreter: /lib/ld-linux-armhf.so.3
```

这表示 `hello_arm` 的 Target rootfs 必须存在匹配的 ARM dynamic loader 和 libc。

所以以后可能出现：

```text
-sh: ./hello_arm: No such file or directory
```

文件明明存在，但 interpreter 不存在。

### 为 Day 4 准备静态版本

```bash
arm-linux-gnueabihf-gcc -static -O0 -g hello.c -o hello_arm_static
file hello_arm_static
readelf -l hello_arm_static | grep INTERP
```

最后一条通常没有输出。

---

## 2.9 Symbol：函数名是怎么进入 ELF 的

```bash
nm -n hello_arm | grep -E ' main$| printf'
```

再：

```bash
arm-linux-gnueabihf-objdump -d hello_arm | less
```

搜索：

```text
<main>
```

现在可以连起来：

```text
C function main()
      ↓
symbol table: main
      ↓
machine code at an address
      ↓
disassembler prints <main>
```

---

## 2.10 Guided Lab：输出工具链基线

执行：

```bash
{
    echo "=== HOST ==="
    uname -m
    gcc --version | head -1

    echo
    echo "=== TARGET TOOLCHAIN ==="
    arm-linux-gnueabihf-gcc --version | head -1
    arm-linux-gnueabihf-gcc -dumpmachine

    echo
    echo "=== HOST ELF ==="
    file hello_x86
    readelf -h hello_x86 | grep -E 'Class:|Machine:|Entry'

    echo
    echo "=== ARM ELF ==="
    file hello_arm
    readelf -h hello_arm | grep -E 'Class:|Machine:|Entry'
} | tee toolchain_baseline.md
```

---

## 2.11 故障实验

### A. 去掉执行权限

```bash
chmod -x hello_x86
./hello_x86
```

错误应是：

```text
Permission denied
```

恢复：

```bash
chmod +x hello_x86
```

### B. 对比 ARM ELF

```bash
./hello_arm
```

这是：

```text
Exec format error
```

必须区分权限阶段与 ELF Loader 阶段。

---

## 2.12 本章验收

不查资料回答：

1. `arm-linux-gnueabihf-gcc` 在哪里运行？生成的东西在哪里运行？
2. `.o` 为什么也是 ELF？
3. Section 和 Segment 的消费者分别是谁？
4. `main()` 为什么不等于 ELF entry point？
5. 文件存在为什么可能报 `No such file or directory`？
6. `file` 和 `readelf -h` 各看什么？

---

## 2.13 原始资料

- `ALI-DRV-1.5.2`
  - **第3章** Linux C 编程入门；
  - **第4章 / 4.3.1-4.3.3** 交叉编译器；
  - 4.3.3 原文通过 `file led.o` 验证 ARM ELF。
  - [PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- `ALI-QUICK-1.8`
  - **第4章**交叉编译；
  - **4.1**通用 ARM 工具链；
  - **4.6**简单 C 文件。
  - [Official 4.x](https://wiki.alientek.com/docs/category/%E7%AC%AC%E5%9B%9B%E7%AB%A0-%E4%BA%A4%E5%8F%89%E7%BC%96%E8%AF%91-3/)
  - [Official 4.6](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/comple_c/)
- Linux `elf(5)`：[man7](https://man7.org/linux/man-pages/man5/elf.5.html)
