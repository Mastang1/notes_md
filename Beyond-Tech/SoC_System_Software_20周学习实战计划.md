# 2026–2027 异构 SoC 平台软件 / Linux BSP & Driver 20 周学习与实战计划

> 适用对象：9 年嵌入式底层开发背景，熟悉 MCU/RTOS/ARM/MCAL/多核 IPC/Yocto，Linux Driver 实战不足  
> 目标岗位：北京地区高级 SoC System Software / Platform Software / Linux BSP & Driver / AI Accelerator System Software  
> 学习周期：20 周  
> 时间预算：每天约 2 小时，建议每周 6 天学习 + 1 天复盘，约 12 小时/周，总投入约 240 小时  
> 核心原则：**不从零重学嵌入式；不堆知识点；围绕真实 SoC 软件执行链，形成“能写、能调、能解释、能证明”的能力。**

---

## 0. 这 20 周到底要解决什么问题

你的问题不是 C、ARM、RTOS 基础薄弱，而是能力链在 Linux Kernel / Driver 这一段存在明显断层。

你现在已经具备：

```text
硬件/IP
  ↓
寄存器
  ↓
Cortex-M / MCU
  ↓
中断 / RTOS
  ↓
MCAL / LIN / CAN
  ↓
Shared Memory / IPC
```

同时也已经接触：

```text
Linux 应用
Yocto
Device Tree
Kernel 编译
IPCF / IPC-SHM
Hailo8 Runtime / Driver / Yocto 移植
```

未来 20 周要补齐中间这座桥：

```text
                    Linux Kernel
                         │
               Driver Model / LDM
                         │
      platform / char / IRQ / DMA / PCIe
                         │
     MMIO / cache / memory barrier / mmap
                         │
       ftrace / perf / kgdb / crash debug
                         │
                      User Space
```

最终形成下面这条职业主线：

```text
                自研异构 SoC
                    │
        ┌───────────┴───────────┐
        │                       │
    Cortex-M/R               Cortex-A
        │                       │
    RTOS / MCAL               Linux
        │                       │
 LIN / CAN / FW          BSP / Driver
        │                       │
        └──── Shared Memory ────┘
                 │
          Mailbox / IRQ
                 │
        Cache / Barrier
                 │
             Multi-Core
                 │
        PCIe / AI Accelerator
                 │
          Runtime / Yocto
```

20 周结束后，你不是要变成“学习过 Linux 驱动的人”，而是要达到：

> **能够从 Boot → Kernel → Driver → IPC → Runtime → User Space 分析、开发和定位 SoC 软件问题的高级嵌入式系统软件工程师。**

---

# 1. 学习环境设计

## 1.1 不建议只用 Windows/WSL2

WSL2 很适合日常 Linux 命令、源码阅读和编译，但你前期需要频繁使用：

- 串口；
- TFTP；
- NFS；
- 开发板与 PC 网络互通；
- 交叉编译；
- U-Boot 网络启动；
- Kernel / DTB 动态替换；
- kgdb / gdb；
- USB/JTAG 等调试设备。

为了减少网络、USB 和文件系统映射带来的额外问题，主环境建议：

```text
Windows Host
   │
   ├── VMware Workstation
   │       │
   │       └── Ubuntu 22.04 LTS
   │                │
   │                ├── gcc / make / git
   │                ├── ARM cross compiler
   │                ├── TFTP
   │                ├── NFS
   │                ├── GDB
   │                ├── kernel source
   │                └── Yocto tools
   │
   ├── 串口工具
   └── JTAG / Trace32
            │
        i.MX6ULL
```

### 推荐理由

Ubuntu 22.04 作为主环境，兼顾现代 Linux 工具与长期可维护性。

正点原子旧 BSP 如果出现宿主机版本兼容问题，不要为了旧 BSP 把整个开发环境降级。优先采用：

```text
Ubuntu 22.04 主环境
        +
Ubuntu 20.04 Docker/容器
        ↓
只用于编译旧 BSP
```

这样可以逐渐建立真正企业化的“可重复构建环境”意识。

---

# 2. 三个平台分别承担什么任务

整个 20 周不要只围绕一块开发板。

## 平台 A：正点原子 i.MX6ULL

负责基础 Linux / BSP / Driver。

重点完成：

- BootROM → U-Boot → Kernel → RootFS；
- 交叉编译；
- TFTP / NFS；
- Device Tree；
- platform driver；
- GPIO；
- IRQ；
- pinctrl；
- clock；
- char/misc driver；
- poll/ioctl/mmap；
- 内核模块；
- ftrace；
- kernel Oops；
- U-Boot。

这是前 12 周的主实验平台。

---

## 平台 B：QEMU

用途不是替代 6ULL，而是作为“可以随便炸”的内核实验机。

适合：

- 编译更新版本 Kernel；
- Kernel module；
- 故意制造 NULL pointer / Oops；
- kgdb；
- lockdep；
- KASAN；
- ftrace；
- kprobe；
- crash 调试；
- 不影响真实板卡的危险实验。

目标：

> 真实板解决“硬件与 BSP”，QEMU 解决“内核机制与调试”。

---

## 平台 C：公司自研 SoC + Hailo8

这是整个学习计划最重要的“生产级项目”。

主要承担：

- A53 Linux；
- MCU/RTOS；
- IPC-SHM/IPCF；
- shared memory；
- cache coherency；
- memory barrier；
- interrupt/mailbox；
- PCIe；
- Hailo8 Linux Driver；
- HailoRT；
- Yocto recipe；
- 系统测试；
- 性能与稳定性分析。

后 8 周学习必须逐渐向这个平台收敛。

---

# 3. 每天 2 小时怎么分

普通学习日：

```text
00:00 - 00:15   回忆昨天知识，不看笔记画执行路径
00:15 - 00:50   学一个核心知识点
00:50 - 01:45   实验 / 写代码 / 调试
01:45 - 02:00   写实验结论 + Git commit
```

原则：

> 理论时间不得长期超过实操时间。

每周至少有一天不学新内容，只做：

- 复盘；
- 画图；
- 写 README；
- 整理问题；
- 重新独立完成实验；
- 面试式口述。

---

# 4. 每周必须留下的工程痕迹

整个计划建立一个 Git 仓库：

```text
soc-system-learning/
│
├── 00_environment/
├── 01_linux_basic/
├── 02_kernel_module/
├── 03_device_tree/
├── 04_platform_driver/
├── 05_char_driver/
├── 06_irq/
├── 07_sync/
├── 08_memory/
├── 09_dma/
├── 10_debug/
├── 11_boot/
├── 12_pcie/
├── 13_hailo/
├── 14_ipc/
├── 15_performance/
├── interview/
└── notes/
```

每周至少交付：

1. 一份 `README.md`；
2. 一份执行流程图；
3. 一份实验代码；
4. 一份实验日志；
5. 一段“我能否不用资料讲明白”的总结。

---

# 5. Phase 1：Linux 基础与开发环境

---

# Week 1：搭建真正可用的 Embedded Linux 开发环境

## 本周目标

从“电脑上能打开 Linux”升级到：

> PC 可以编译程序 → 交叉编译 → 下载到 6ULL → 运行 → GDB 调试 → NFS/TFTP 传输。

这是以后所有 Driver 实验的基础。

## Day 1：Ubuntu 环境

完成：

- VMware 安装；
- Ubuntu 22.04；
- Git；
- gcc/g++；
- make；
- cmake；
- vim；
- VS Code Remote；
- ssh；
- tmux；
- tree；
- htop。

必须理解：

```text
Host gcc
≠
Target ARM gcc
```

实验：

```bash
gcc hello.c -o hello_x86
file hello_x86
readelf -h hello_x86
```

观察：

- ELF；
- Machine；
- Entry point；
- section；
- segment。

---

## Day 2：交叉编译

安装 ARM GNU toolchain。

完成：

```bash
arm-linux-gnueabihf-gcc hello.c -o hello_arm
file hello_arm
readelf -h hello_arm
```

思考：

> 为什么 x86 Ubuntu 不能直接执行 ARM ELF？

重新建立：

```text
Source
  ↓
Preprocessor
  ↓
Compiler
  ↓
Assembler
  ↓
Object
  ↓
Linker
  ↓
ELF
```

结合你熟悉的 MCU linker script 做知识迁移。

---

## Day 3：6ULL 串口与网络

完成：

- USB-TTL；
- U-Boot console；
- Linux console；
- 设置开发板 IP；
- ping Ubuntu；
- ping Host；
- SSH。

输出一张网络图：

```text
Windows
  │
VMware bridge
  │
Ubuntu
  │
Ethernet
  │
i.MX6ULL
```

---

## Day 4：TFTP

搭建 TFTP Server。

验证：

```text
Ubuntu
   │
   └── zImage / dtb
          ↓ TFTP
       U-Boot
```

做到：

> 修改 kernel / DTB 后不再反复烧 SD/eMMC。

---

## Day 5：NFS Root / NFS 目录

至少完成 NFS 共享目录。

建议后续实验目录：

```text
/home/user/nfs/
```

开发板：

```bash
mount -t nfs <ubuntu-ip>:/home/user/nfs /mnt/nfs
```

理解 NFS 在 BSP 开发里的意义：

> 修改程序 → 编译 → 开发板直接运行。

---

## Day 6：Git 工程化

完成：

- init；
- branch；
- commit；
- diff；
- tag；
- `.gitignore`；
- patch。

练习：

```bash
git format-patch
git apply
git am
```

这与 Kernel / Yocto 开发高度相关。

---

## Week 1 验收

必须能独立完成：

```text
PC写代码
 ↓
交叉编译
 ↓
NFS/TFTP
 ↓
6ULL
 ↓
运行
 ↓
GDB/日志调试
```

### 周交付物

`00_environment/README.md`

---

# Week 2：Linux 用户态基础，但只学 Driver 工程师需要的部分

## 核心目标

理解：

```text
Process
Virtual Memory
File Descriptor
System Call
ELF
/proc
/sys
```

不学习 shell 花活。

---

## Day 1：进程与线程

掌握：

- process；
- thread；
- PID；
- fork；
- exec；
- wait；
- pthread。

重点建立：

```text
Application
  ↓
libc
  ↓
syscall
  ↓
Kernel
```

---

## Day 2：虚拟地址空间

写程序打印：

- text；
- global；
- static；
- heap；
- stack；
- shared library。

查看：

```bash
cat /proc/<pid>/maps
pmap <pid>
```

将 MCU memory map 与 Linux process memory 做对比。

---

## Day 3：文件描述符

实验：

```c
open()
read()
write()
close()
```

理解：

```text
fd
 ↓
struct file
 ↓
file_operations
```

先不要看太深源码。

只建立用户态到 Driver 的入口意识。

---

## Day 4：strace

必须熟练：

```bash
strace ./app
strace -f
strace -tt
strace -T
```

观察：

```text
openat
read
write
ioctl
mmap
poll
```

这将是后续 HailoRT 分析的重要工具。

---

## Day 5：ELF 工具

必须熟练：

```bash
readelf
objdump
nm
addr2line
ldd
file
strings
```

结合你熟悉的 linker：

分析：

```text
ELF Header
Program Header
Section Header
Symbol
Relocation
```

---

## Day 6：GDB

掌握：

- breakpoint；
- watch；
- bt；
- frame；
- info registers；
- x；
- disassemble；
- core dump。

### Week 2 验收

能回答：

> 一个 `read(fd)` 最终是如何进入 Linux Kernel 的？

> ELF section 和 segment 有什么区别？

> 用户进程为什么不能直接访问物理地址？

---

# Week 3：Kernel 编译、Kbuild 和 Kernel Module

## 目标

从“会编译内核”升级为：

> 清楚 Kernel 的构建产物、配置系统、模块加载和符号体系。

---

## 重点知识

```text
Kconfig
  ↓
.config
  ↓
Makefile/Kbuild
  ↓
vmlinux
  ↓
zImage
  ↓
modules
  ↓
dtb
```

---

## 实验 1：完整编译 6ULL Kernel

记录：

```bash
make xxx_defconfig
make menuconfig
make -j...
```

找到：

- vmlinux；
- System.map；
- zImage；
- `.ko`；
- DTB。

---

## 实验 2：Hello Kernel Module

实现：

```c
module_init()
module_exit()
pr_info()
```

操作：

```bash
insmod
lsmod
rmmod
modinfo
dmesg
```

---

## 实验 3：模块参数

实现：

```text
module_param
```

理解模块加载时参数如何进入 Kernel。

---

## 实验 4：Kernel Symbol

观察：

```bash
cat /proc/kallsyms
nm module.ko
```

理解：

```text
EXPORT_SYMBOL
EXPORT_SYMBOL_GPL
```

---

## 本周必须知道

Kernel module 与 MCU 静态固件最大区别：

```text
运行中的 Kernel
       │
       ├── symbol table
       │
       └── dynamically load .ko
```

---

# Week 4：Device Tree + Linux Driver Model

这是第一阶段最关键的一周。

## 目标

把你之前“知道 LDM 原理”的知识变成真实执行链。

必须最后能画：

```text
DTS
 ↓
DTB
 ↓
Bootloader
 ↓
Kernel unflatten
 ↓
device_node
 ↓
platform_device
 ↓
platform_driver
 ↓
of_match_table
 ↓
probe()
```

Linux 官方文档把 platform device 描述为 SoC 内部这类可由 CPU 直接寻址的设备的常用抽象；设备与驱动匹配后由 driver core 调用 `probe()`。因此这一周重点不是背结构体，而是亲自观察匹配与 probe 执行。

---

## 实验

### 实验 A：增加自己的 DTS Node

例如：

```dts
mydev@02000000 {
    compatible = "student,mydev";
    reg = <...>;
    interrupts = <...>;
};
```

最开始地址可以只用于学习资源解析，不一定真正控制硬件。

---

### 实验 B：platform_driver

实现：

```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "student,mydev" },
    {}
};
```

以及：

```c
probe()
remove()
```

---

### 实验 C：资源读取

学习：

```c
platform_get_resource()
devm_ioremap_resource()
platform_get_irq()
```

---

## Week 4 验收

闭眼画出：

```text
DTS → device → match → probe
```

并回答：

> 为什么 platform bus 不需要像 PCIe 一样扫描设备？

> `compatible` 到底是谁跟谁匹配？

---

# 6. Phase 2：Driver 核心能力

---

# Week 5：VFS 与字符设备——只从“怎么写 Driver”理解 VFS

## 目标

不再继续抽象地研究 VFS。

直接完成：

```text
User
 open/read/write/ioctl
        │
        ↓
       VFS
        │
        ↓
 file_operations
        │
        ↓
     Driver
        │
        ↓
       HW
```

---

## 实验 1：miscdevice

先用最简单的：

```c
struct file_operations
```

实现：

- open；
- release；
- read；
- write。

---

## 实验 2：ioctl

设计一个最小协议：

```text
GET_VALUE
SET_VALUE
RESET
```

学习：

```c
_IO
_IOR
_IOW
_IOWR
copy_from_user
copy_to_user
```

---

## 实验 3：sysfs 与 debugfs

比较：

```text
/dev
/sys
/sys/kernel/debug
```

理解三种接口分别适合什么。

---

## Week 5 验收

自己实现：

```text
mydrv.ko
  +
test_app
```

User Space 可以：

```text
open
read
write
ioctl
```

驱动内部可看到完整日志。

---

# Week 6：用真实 6ULL 外设做 platform driver

## 推荐实验：GPIO LED + KEY

你 MCU 经验丰富，因此这一周重点不是 GPIO 原理，而是：

> Linux 如何把 GPIO 这个硬件资源纳入 Device Tree、pinctrl 和 driver model。

---

## 必须搞清

```text
IOMUXC
 ↓
pinctrl
 ↓
GPIO Controller
 ↓
Device Tree
 ↓
gpiod API
 ↓
Driver
```

---

## 实验

禁止只抄教程。

要求自己完成：

1. 找原理图；
2. 查 Pin；
3. 查 Reference Manual；
4. 找 GPIO controller；
5. 修改 DTS；
6. 编译 DTB；
7. 写 Driver；
8. 控 LED；
9. 读 KEY。

---

## 特别训练

出现错误时禁止直接问 AI。

按照：

```text
Hardware
 ↓
DTS
 ↓
Kernel Log
 ↓
sysfs
 ↓
Driver
```

逐层查。

这是建立真正 BSP 调试能力的开始。

---

# Week 7：IRQ、Bottom Half、等待队列、poll

## 目标

把 MCU 中断知识迁移到 Linux。

你的 MCU 模型：

```text
IRQ
 ↓
ISR
 ↓
RTOS event
 ↓
Task
```

Linux 模型：

```text
IRQ
 ↓
Top Half
 ↓
threaded irq/workqueue
 ↓
wait_queue
 ↓
process
```

---

## 实验

KEY GPIO interrupt：

Driver：

```text
request_irq
ISR
wait_queue
poll
```

User：

```text
poll()
```

做到：

> 用户程序不循环查询，按键后内核唤醒应用。

---

## 必须理解

- hard IRQ context；
- process context；
- atomic context；
- 为什么 ISR 不能 sleep；
- threaded IRQ；
- workqueue；
- tasklet 的历史位置；
- completion；
- wait queue。

---

# Week 8：并发、锁与 Memory Ordering

这是向 System Software 转型的重要一周。

## 内容

- spinlock；
- mutex；
- atomic；
- semaphore；
- completion；
- rwlock 概念；
- lock-free 只理解，不强求实现；
- compiler barrier；
- CPU memory barrier。

---

## 实验 A：Race Condition

Driver 中实现两个并发访问路径。

故意不加锁。

制造统计错误。

然后分别使用：

```text
mutex
spinlock
atomic
```

修复。

---

## 实验 B：生产者消费者

做一个 kernel ring buffer：

```text
IRQ / writer
      ↓
 ring buffer
      ↓
 reader
```

对比你已有 MCU ring buffer。

---

## Week 8 面试题

必须能回答：

> spinlock 和 mutex 为什么不能随便替换？

> interrupt context 里为什么不能 mutex？

> volatile 为什么不能解决多核同步？

---

# 7. Phase 3：Linux 内存、DMA 和高级调试

---

# Week 9：Linux 内存体系

## 目标

把 MCU physical memory 心智模型升级为 Linux：

```text
User VA
   ↓
Page Table
   ↓
Physical Memory

Kernel VA
   ↓
Page Table
   ↓
Physical Memory

Device Register
Physical Address
   ↓
ioremap
   ↓
Kernel Virtual Address
```

---

## 学习

- VA；
- PA；
- page；
- page table；
- TLB；
- kmalloc；
- vmalloc；
- alloc_pages；
- ioremap；
- mmap；
- `/proc/iomem`；
- `/proc/meminfo`。

---

## 实验

Driver 分配 buffer：

```c
kmalloc()
```

User 通过：

```c
mmap()
```

映射。

观察地址。

回答：

> User VA、Kernel VA、PA 是否相同？

---

# Week 10：DMA + Cache Coherency

这是高级 SoC 岗位非常关键的一周。

Linux DMA API 需要区分 CPU virtual address、CPU physical address 与 device 使用的 DMA address；存在 IOMMU 时 DMA address 还可能再次经过地址转换。因此 Driver 不应该自己假定“物理地址就是设备地址”，而应使用标准 DMA API。

## 必须理解

```text
CPU VA
 ↓
CPU PA
 ↓
IOMMU(optional)
 ↓
DMA Address
 ↓
Device
```

以及：

```text
dma_alloc_coherent
dma_map_single
dma_unmap_single
dma_sync_*()
```

---

## 两类 DMA

### coherent DMA

```text
CPU <----> Memory <----> Device
```

软件不需要显式进行普通 cache sync。

### streaming DMA

存在明确 ownership 切换：

```text
CPU owns buffer
      ↓
dma_map
      ↓
Device owns buffer
      ↓
dma_unmap / sync
      ↓
CPU owns buffer
```

---

## 6ULL 实验策略

不要求为了学习 DMA 强行手写复杂 SDMA Driver。

执行三个层级：

### Level 1

阅读 6ULL 一个使用 DMAEngine 的现有 Driver。

### Level 2

通过：

```text
dmesg
/proc/interrupts
debugfs
```

观察 DMA。

### Level 3

如果时间允许，编写 DMAEngine client 或对 UART/SPI DMA 路径进行实验。

真正重要的是搞清：

> CPU、cache、DDR、DMA controller、device 的 ownership 与地址关系。

---

# Week 11：大厂常用 Linux 调试方法 I

这一周不学 Driver 功能，专门训练“定位问题”。

Linux Kernel 官方调试文档明确把 dynamic debug、ftrace、perf 等作为常用内核调试/分析工具；ftrace 本身不仅可以跟踪函数，也可以分析 IRQ/preemption/wakeup latency。

## 调试能力梯度

### Level 0

```text
printk
dmesg
```

### Level 1

```text
dynamic_debug
```

### Level 2

```text
ftrace
trace-cmd
KernelShark
```

### Level 3

```text
perf
```

### Level 4

```text
kgdb
kdb
crash
```

### Level 5

```text
kprobe
eBPF
KASAN
lockdep
```

20 周目标达到 Level 4。

---

## 本周实验 1：dynamic debug

学习：

```text
/sys/kernel/debug/dynamic_debug/control
```

---

## 实验 2：ftrace function_graph

追踪你自己的 Driver：

```text
open
ioctl
IRQ
```

形成：

```text
user ioctl
 ↓
sys_ioctl
 ↓
driver ioctl
```

---

## 实验 3：制造 Kernel Oops

QEMU 中写故障模块：

```c
int *p = NULL;
*p = 1;
```

然后使用：

```text
dmesg
addr2line
objdump
```

定位源码。

---

## 实验 4：kgdb

QEMU 优先。

理解：

```text
Target Kernel
    │ serial/network
    ↓
Host GDB
```

官方文档中，kdb 更偏向控制台级检查，kgdb 则用于 Linux Kernel 源码级调试。

---

# Week 12：Boot Chain / U-Boot / BSP

## 目标

建立完整 SoC 启动链：

```text
POR
 ↓
BootROM
 ↓
SPL(optional)
 ↓
U-Boot
 ↓
Kernel
 ↓
DTB
 ↓
init
 ↓
RootFS
 ↓
User Space
```

---

## 6ULL 必做实验

1. 编译 U-Boot；
2. 修改 U-Boot banner；
3. 修改 env；
4. 修改 bootargs；
5. TFTP boot；
6. 切换不同 DTB；
7. 故意设置错误 root=；
8. 分析 kernel panic；
9. 恢复。

---

## 必须理解

```text
bootcmd
bootargs
console
root=
earlycon
init=
```

---

## Week 12 阶段验收

此时必须做到：

> 给你一块能进入 U-Boot 的 6ULL，你能够自己让 Kernel 启动起来，并能修改 DTB、加载 Driver、定位普通启动失败。

---

# 8. Phase 4：PCIe + Hailo8，进入生产级项目

---

# Week 13：PCIe 从硬件模型升级到 Linux PCI Subsystem

6ULL 不承担这一周主要实验。

主平台切换到公司 SoC + Hailo8。

## 心智模型

```text
SoC
 │
PCIe RC
 │
Link
 │
Hailo8 EP
 │
Configuration Space
 │
BAR
 │
MMIO
 │
DMA
 │
MSI/MSI-X
```

---

## Linux 模型

```text
PCI Core
   │
Enumeration
   │
struct pci_dev
   │
pci_driver
   │
id_table
   │
probe
   │
pci_enable_device
   │
pci_request_regions
   │
pci_iomap
   │
DMA
```

---

## 实操

至少执行并解释：

```bash
lspci
lspci -vv
lspci -xxx
cat /proc/iomem
cat /proc/interrupts
```

找到 Hailo8：

- Vendor ID；
- Device ID；
- BAR；
- IRQ；
- PCIe speed；
- link width。

---

# Week 14：Hailo8 Driver → HailoRT 调用链

这是你未来简历最有价值的一周之一。

## 目标

画出：

```text
AI Application
      ↓
   HailoRT
      ↓
 open/ioctl/mmap
      ↓
 Hailo Kernel Driver
      ↓
 PCIe / DMA / IRQ
      ↓
   Hailo8
```

---

## 方法

不要一上来通读所有源码。

从运行行为反查：

```bash
strace -f application
```

记录：

```text
open()
ioctl()
mmap()
poll()
```

然后进入 Kernel Driver 找对应：

```text
file_operations
ioctl handler
mmap handler
interrupt handler
DMA
```

---

## 输出

写文档：

`Hailo8_Linux_Driver_Runtime_Path.md`

必须包含：

1. Driver probe；
2. BAR；
3. IRQ；
4. DMA；
5. char device；
6. ioctl；
7. Runtime；
8. inference 流程。

---

# Week 15：Hailo8 + Yocto 工程化

这一周把你当前正在干的事情彻底变成面试优势。

## 重点

```text
Recipe
 │
SRC_URI
 │
Fetch
 │
Patch
 │
Configure
 │
Compile
 │
Install
 │
Package
 │
Image
```

---

## 必须搞清

- DEPENDS；
- RDEPENDS；
- native；
- target；
- sysroot；
- `${S}`；
- `${B}`；
- `${D}`；
- WORKDIR；
- DL_DIR；
- SSTATE；
- package split；
- kernel module packaging；
- service；
- image install。

---

## 实战

以 Hailo8 为对象，输出：

```text
meta-company/
   recipes-hailo/
      hailort/
      hailo-driver/
```

即使公司实际层结构不同，也要自己画出 dependency graph。

重点回答：

> 为什么外网能编、内网不能编？

> BitBake fetch 与 CMake FetchContent 是两套什么机制？

> runtime dependency 与 build dependency 有什么区别？

---

# 9. Phase 5：异构多核系统软件

---

# Week 16：重新学习 IPCF——这一次站在系统软件角度

之前你的目标是“调通 IPC”。

现在目标升级：

> 能设计和解释异构 SoC IPC。

---

## 完整链路

```text
Cortex-M
    │
Producer
    │
Cache Clean
    │
Memory Barrier
    │
Shared DDR
    │
Mailbox / IRQ
    ↓
Cortex-A53
    │
IRQ
    │
Cache Invalidate
    │
Consumer
    │
Linux Driver
    │
User
```

---

## 分析 IPC-SHM

不要全读。

重点找：

- shared memory allocation；
- descriptor；
- ring；
- producer index；
- consumer index；
- IRQ；
- cache；
- barrier；
- synchronization；
- timeout；
- reset/recovery。

---

## 输出

`IPCF_architecture.md`

要求能回答：

> 两个核同时修改一个 ring index 会发生什么？

> 为什么 volatile 不够？

> cache line false sharing 是什么？

> barrier 保证的到底是什么？

---

# Week 17：IPC 性能、可靠性与系统测试

你已经有自动测试开发经验，这一周把它升级成“平台软件工程能力”。

## 性能指标

至少测：

- latency；
- throughput；
- packet size；
- CPU usage；
- drop；
- interrupt count；
- error rate。

---

## Reliability Test

设计：

```text
Core Reset
IPC Restart
Remote Core Timeout
Malformed packet
Ring full
Ring empty
High frequency
Long-term pressure
```

---

## 自动化

用 Python 输出：

```text
Test Case
 ↓
Run
 ↓
Collect log
 ↓
Parse
 ↓
Statistics
 ↓
Report
```

不要只“测试通过”。

要输出定量结果。

---

# 10. Phase 6：性能分析、简历和面试

---

# Week 18：大厂常用 Linux 调试方法 II

这一周开始达到真正高级岗位的调试表达。

## perf

掌握：

```bash
perf stat
perf top
perf record
perf report
```

理解：

- cycles；
- instructions；
- context-switches；
- cache-misses。

---

## ftrace

掌握：

```text
function
function_graph
irq events
sched events
```

---

## trace-cmd / KernelShark

完成一次：

> IPC 或 Driver latency 时间线分析。

---

## /proc 与 /sys 调试

熟练：

```text
/proc/interrupts
/proc/iomem
/proc/meminfo
/proc/slabinfo
/sys/kernel/debug
/sys/bus
/sys/class
/sys/devices
```

---

## 高级工具形成认知

这一周不要求精通，但必须知道使用场景：

```text
KASAN       内存越界/UAF
kmemleak    内核内存泄漏
lockdep     锁依赖/死锁
pstore      panic 后保存日志
crash       vmcore 分析
kgdb        Kernel source debug
kprobe      动态探针
eBPF        动态观测
```

---

# Week 19：项目收口 + 面试 Coding

从这周开始禁止继续无目的扩展技术栈。

## 最终准备三个项目故事

### 项目 1：异构 SoC IPC

卖点：

```text
A53 Linux
+
MCU RTOS
+
Shared Memory
+
Interrupt
+
IPC
+
系统测试
```

---

### 项目 2：Hailo8 PCIe AI Accelerator

卖点：

```text
Yocto
+
PCIe
+
Linux Driver
+
Runtime
+
Dependency Integration
```

---

### 项目 3：Linux BSP / Driver Lab

卖点不是工作经历，而是证明你补齐了 Driver 能力：

```text
i.MX6ULL
+
U-Boot
+
Device Tree
+
platform driver
+
IRQ
+
DMA
+
mmap
+
ftrace/kgdb
```

---

## Coding 最小题库

每天 30 分钟。

必须手写：

1. 单链表；
2. 双链表；
3. intrusive list；
4. stack；
5. queue；
6. ring buffer；
7. producer-consumer；
8. hash table；
9. binary search；
10. BST；
11. tree DFS/BFS；
12. bitmap；
13. memcpy；
14. memmove；
15. strlen/strcmp；
16. endian convert；
17. bit operations；
18. LRU；
19. timer queue 基本思想；
20. packet reorder window。

你的滑动窗口项目要重点复习，因为这是非常好的系统设计面试题。

---

# Week 20：模拟面试 + 简历 + 投递

实际投递不要等到 Week 20。

建议：

```text
Week 16：开始看 JD
Week 17：试投 2~3 家
Week 18：根据面试反馈补洞
Week 19：正式投递
Week 20：集中面试
```

---

## 简历岗位标题

优先使用：

```text
高级 SoC 系统软件工程师
Senior SoC System Software Engineer

高级平台软件工程师
Senior Platform Software Engineer

Linux BSP / Driver Engineer
```

不要继续把自己主标签写成：

```text
LIN工程师
MCAL工程师
```

---

# 11. 20 周总览表

| 周 | 主主题 | 主要平台 | 必须交付 |
|---|---|---|---|
| W1 | Linux 开发环境 | Ubuntu + 6ULL | TFTP/NFS/交叉编译闭环 |
| W2 | Linux 用户态基础 | Ubuntu + 6ULL | ELF/strace/GDB |
| W3 | Kernel/Kbuild/module | 6ULL | 自写 .ko |
| W4 | DTS + Driver Model | 6ULL | platform driver |
| W5 | VFS/char/ioctl | 6ULL | char/misc driver |
| W6 | GPIO/pinctrl | 6ULL | LED/KEY driver |
| W7 | IRQ/poll/waitqueue | 6ULL | interrupt-driven driver |
| W8 | lock/concurrency | 6ULL/QEMU | ring + locking |
| W9 | MMU/memory/mmap | 6ULL/QEMU | mmap driver |
| W10 | DMA/cache | 6ULL | DMA 分析实验 |
| W11 | ftrace/kgdb/Oops | QEMU/6ULL | Oops 定位报告 |
| W12 | U-Boot/Boot chain | 6ULL | 网络启动/启动故障定位 |
| W13 | PCIe | 公司 SoC | Hailo PCIe 枚举报告 |
| W14 | Hailo Driver/Runtime | 公司 SoC | 调用链分析 |
| W15 | Yocto/Hailo | 公司 SoC | dependency graph |
| W16 | IPC 架构 | 公司 SoC | IPCF 架构文档 |
| W17 | IPC 性能可靠性 | 公司 SoC | benchmark/report |
| W18 | perf/ftrace | 公司 SoC | 性能 trace |
| W19 | 项目/Coding | 全部 | 三个项目故事 |
| W20 | 面试/简历 | — | 可正式投递版本 |

---

# 12. 你的 Linux Driver 学习优先级

## P0：必须掌握

```text
Kernel module
Kbuild
Device Tree
platform driver
probe/remove
MMIO
IRQ
char/misc
ioctl
poll
mmap
waitqueue
mutex
spinlock
atomic
DMA API
cache coherency
PCIe
ftrace
perf
U-Boot
```

---

## P1：应该了解

```text
workqueue
completion
sysfs
debugfs
pinctrl
clock
regulator
DMAEngine
kgdb
crash
lockdep
KASAN
```

---

## P2：半年内不作为重点

```text
USB Driver 深入
Display DRM
V4L2
ALSA
Wi-Fi Driver
Filesystem implementation
Network stack源码
eBPF 深入开发
复杂 Scheduler 源码
```

除非目标 JD 明确要求。

---

# 13. 大厂调试能力矩阵

## User Space

| 工具 | 目标 |
|---|---|
| gdb | 必须熟练 |
| strace | 必须熟练 |
| readelf | 必须熟练 |
| objdump | 必须熟练 |
| nm | 必须熟练 |
| addr2line | 必须熟练 |
| core dump | 必须熟练 |

---

## Kernel

| 工具 | 20 周目标 |
|---|---|
| dmesg/printk | 熟练 |
| dynamic_debug | 熟练 |
| ftrace | 熟练 |
| trace-cmd | 会实际定位 |
| perf | 会实际定位 |
| kgdb | 能完成一次 |
| crash | 理解并完成基本分析 |
| KASAN | 会开启/理解报告 |
| lockdep | 会使用 |
| kprobe | 了解/做一个实验 |
| eBPF | 知道使用场景 |

---

## Hardware

继续保持现有优势：

```text
Oscilloscope
Logic Analyzer
JTAG
Trace32/Lauterbach
Register Dump
```

但面试表达要形成：

```text
软件日志
 ↓
Kernel Trace
 ↓
寄存器
 ↓
总线波形
```

完整定位路径。

---

# 14. 学习时必须避免的 8 个坑

## 1. 不要连续两周只看视频

如果连续两周没有 Git commit，学习方式已经失败。

## 2. 不要沉迷源码

Linux Kernel 不需要从 `start_kernel()` 开始通读。

按问题跟踪源码。

例如：

```text
ioctl 为什么进到这里？
IRQ 怎么唤醒 poll？
PCI probe 为什么没调用？
```

然后追源码。

---

## 3. 不要用 AI 直接生成完整 Driver

正确方式：

1. 自己设计；
2. 自己写骨架；
3. 编译；
4. 看错误；
5. 查文档；
6. AI 用来 code review。

否则半年以后仍然不会写。

---

## 4. 不要重复 MCU 知识

GPIO/SPI/I2C 的硬件时序你已经会。

学习重点必须放到：

```text
Linux abstraction
driver model
resource management
concurrency
debug
```

---

## 5. 不要学 AUTOSAR

本轮跳槽暂不投资。

除非后续出现明确目标岗位。

---

## 6. 不要把“编译成功”当项目完成

每一个实验至少包含：

```text
正常路径
异常路径
日志
性能
原理
```

---

## 7. 不要只写笔记

必须形成：

```text
代码 + 日志 + 图 + 结论
```

---

## 8. 不要等全部学完才投简历

Week 16 开始投递。

真实面试本身就是最高价值的能力扫描器。

---

# 15. 每四周一次能力验收

## Week 4

不看资料解释：

```text
DTS → platform_device → match → probe
```

并写出 platform driver。

---

## Week 8

实现：

```text
IRQ
 ↓
kernel ring buffer
 ↓
poll
 ↓
user process
```

并正确处理并发。

---

## Week 12

从 U-Boot 启动 6ULL：

```text
TFTP kernel
TFTP dtb
NFS/rootfs
```

并定位至少一个人为制造的启动故障。

---

## Week 16

完整解释：

```text
HailoRT
 ↓
Kernel Driver
 ↓
PCIe
 ↓
DMA
 ↓
Hailo8
```

以及：

```text
MCU
 ↓
Shared Memory
 ↓
IRQ
 ↓
A53 Linux
```

---

## Week 20

随机抽一份目标 JD。

30 分钟内完成：

1. 能力匹配；
2. 缺口判断；
3. 项目映射；
4. 2 分钟自我介绍；
5. 20 分钟技术项目深挖。

---

# 16. 20 周结束后的最低能力标准

必须做到以下问题不需要 AI 才能回答：

### Linux

- 一个进程执行 `ioctl()` 如何进入 Driver？
- `mmap()` 怎么让 User 访问 Driver buffer？
- `platform_driver` 为什么会调用 probe？
- Device Tree 和 Driver 是怎么关联的？
- IRQ handler 为什么不能 sleep？
- mutex/spinlock 有什么本质差异？
- `kmalloc` 和 `vmalloc` 有什么区别？
- MMIO 为什么用 `ioremap`？
- DMA address 为什么不一定等于 PA？
- cache coherence 为什么会影响 DMA/IPC？

### BSP

- SoC 从复位到 Linux shell 的流程？
- U-Boot 向 Kernel 传什么？
- DTB 在什么时候被 Kernel 解析？
- Kernel panic 时怎么定位？
- 如何替换 Kernel/DTB 而不重新烧镜像？

### PCIe

- Enumeration 做什么？
- Config Space 是什么？
- BAR 是什么？
- RC 如何访问 EP？
- EP 如何 DMA Host 内存？
- MSI 与传统 IRQ 有什么区别？
- Linux PCI Driver probe 如何发生？

### IPC

- Shared memory 为什么还需要 IRQ？
- 为什么需要 ring？
- 为什么需要 barrier？
- cache clean/invalidate 为什么重要？
- producer/consumer 如何避免 race？
- remote core reset 怎么恢复？

### Debug

至少能实际使用：

```text
gdb
strace
readelf
objdump
addr2line
dmesg
dynamic_debug
ftrace
trace-cmd
perf
kgdb
```

---

# 17. 推荐资料顺序

不要一次下载几十本书。

## 第一优先：正点原子 i.MX6ULL 官方资料

用于：

- 环境；
- Boot；
- U-Boot；
- DTS；
- GPIO；
- Kernel Driver 入门。

官方在线资料：

https://wiki.alientek.com/docs/category/imx6u-%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C-3/

注意：

> 正点原子教程用于“快速建立实验闭环”，不能成为最终知识边界。接口、机制和最佳实践最终以 Kernel 官方文档和当前内核代码为准。

---

## 第二优先：Linux Kernel 官方文档

### Driver Model

https://docs.kernel.org/driver-api/driver-model/

### Platform Driver

https://docs.kernel.org/driver-api/driver-model/platform.html

### DMA API

https://docs.kernel.org/core-api/dma-api.html

### DMA HOWTO

https://docs.kernel.org/core-api/dma-api-howto.html

### Tracing

https://docs.kernel.org/trace/

### KGDB/KDB

https://docs.kernel.org/process/debugging/kgdb.html

---

## 第三优先：源码

优先顺序：

```text
Documentation/
 ↓
include/linux/
 ↓
drivers/<subsystem>
 ↓
核心实现
```

不要反过来。

---

# 18. 最终项目资产

20 周结束后，建议留下下面 5 份真正可以反复复习的个人技术资产：

```text
1. iMX6ULL_Linux_BSP_Driver_Lab.md

2. Linux_Driver_Execution_Path.md

3. Hailo8_PCIe_Driver_Runtime_Architecture.md

4. Heterogeneous_SoC_IPC_Architecture.md

5. Linux_Debug_Playbook.md
```

这五份东西比“看完一本 Linux Driver 书”更有价值。

---

# 19. 最终职业心智模型

以后再碰到一个新 SoC，不要再按“这是 SPI、这是 I2C、这是某个 Driver”分散理解。

统一成下面的模型：

```text
                         SoC
                          │
                  Hardware Resource
                          │
           ┌──────────────┴──────────────┐
           │                             │
       CPU-visible                   DMA-capable
           │                             │
         MMIO                         Memory
           │                             │
    Device Tree / PCI                 IOMMU
           │                             │
           └──────────┬──────────────────┘
                      │
                 Linux Driver
                      │
          ┌───────────┼───────────┐
          │           │           │
        IRQ         ioctl       mmap
          │           │           │
          └───────────┼───────────┘
                      │
                  User Space
                      │
                    Runtime
                      │
                  Application
```

异构 SoC 再增加一条：

```text
Linux/A-Core
     │
   Driver
     │
Shared Memory
     │
Mailbox / IRQ
     │
RTOS/M-Core
```

你的未来竞争力，来自能够把上面两张图放在同一个脑子里。

---

# 20. 一句话执行原则

未来 20 周每学一个知识点，都问自己三个问题：

> **它在真实 SoC 的哪一层？**

> **我能不能写一个实验证明我理解？**

> **出了问题，我用什么工具定位？**

只要持续按这三个问题执行，20 周后你与传统“只做 MCU/MCAL”的工程师会形成非常明显的能力差异，同时又不会失去你原来 ARM、RTOS、多核 IPC 和硬件调试的优势。
