# Chapter 25 - When the Kernel Crashes: Oops, Address, Symbol and Source Line

> Week 4 / Day 4 - 在可恢复环境中建立 Kernel crash -> source 的证据链。

[← Part README](README.md) · [← Previous](ch24_module_symbols_vermagic.md) · [Next →](ch26_zephyr_scheduling.md)

## 25.1 为什么必须主动经历一次 Oops

大厂底层岗位的调试价值不在“代码从不崩”，而在 crash 后能迅速回答：

```text
谁崩了？
在哪个 context？
PC/LR 在哪里？
调用栈是什么？
这个地址对应哪一版 binary/source？
```

本章优先 QEMU 或明确可恢复的测试板环境。不要在公司关键设备上做故障注入。

## 25.2 Oops、panic、userspace segfault 不是同一个层次

- userspace segfault：某 process 违规，Kernel 通常只杀这个 process；
- Kernel Oops：Kernel 检测到异常，打印诊断；系统可能继续但状态未必可信；
- Kernel panic：Kernel 判断不能安全继续或策略要求 panic。

所以看到“NULL pointer”先问发生在 user 还是 kernel mode。

## 25.3 最小故障 Module：让错误足够单纯

示例：

```c
static int __init crash_init(void)
{
    volatile int *p = NULL;
    pr_info("crash_lab: about to fault\n");
    *p = 0x1234;
    return 0;
}
```

只在 QEMU/可恢复目标运行。编译保留 debug info，与对应 `vmlinux`/`.ko` 一起归档。

## 25.4 Oops 第一遍不要急着查源码：先读结构

重点字段因架构/版本不同而变化，但通常关注：

```text
exception/fault type
PC / LR
register dump
Call Trace / backtrace
loaded modules
taint flags
Kernel version
```

先保存完整 dmesg：

```bash
dmesg > oops_full.log
```

截一行永远不够。

## 25.5 从地址回源码：必须绑定正确二进制

### module 内地址

可先：

```bash
${CROSS_COMPILE}nm -n crash_lab.ko
${CROSS_COMPILE}objdump -dS crash_lab.ko | less
```

由于 module runtime load base/relocation，实际地址映射要结合 module section load address/trace 信息。

### built-in kernel

对应 vmlinux：

```bash
${CROSS_COMPILE}addr2line -e vmlinux -fip <address>
```

但地址必须是与该 vmlinux 匹配且经过必要地址处理的有效 symbol address。

## 25.6 `decode_stacktrace.sh` / symbolized trace 思路

不同 Kernel tree 提供 helper script/配置，可将 stack addresses 与 vmlinux/source 映射。不要死背某版本脚本参数，核心输入始终是：

```text
Oops log + exact vmlinux + exact modules + source tree
```

缺一项，调试质量都会下降。

## 25.7 Worked Example：把故障前日志和故障栈连起来

在 fault 前打印 unique marker：

```c
pr_info("crash_lab: stage=before-null-write\n");
```

Oops 后问：

- marker 是否出现？
- init 是否完成？
- fault function 是否在 call trace？
- module load 是否被回滚/系统是否继续？

这训练“timeline + crash state”，而不是只盯 PC。

## 25.8 Guided Lab：用 `objdump -dS` 对照 C 与 ARM 指令

找到故障写内存的 instruction，观察 NULL base register/访问。把异常寄存器值与 instruction operands 对起来。

你已有 MCU fault/JTAG 经验，这一步是直接迁移：Linux 只是多了 symbols/module/runtime context。

## 25.9 Independent Challenge：写一页 Oops Triage SOP

固定为：

1. 保存完整 serial/dmesg；
2. 保存 uname/config/module list；
3. 锁定 exact vmlinux/.ko；
4. 判断 user/kernel context；
5. 读 fault type/PC/LR/call trace；
6. symbolicate；
7. 反汇编验证；
8. 最小复现。

## 25.10 下一章：Linux 用 crash 强化“运行时上下文”，Zephyr 侧开始从 API 使用进入 scheduler 行为

Chapter 26 做三个不同优先级 thread + semaphore/queue/timer，用日志/trace 证明 ready/block/wakeup，而不是只知道 `k_sem_take()` 怎么调用。

## References and manuals

### Linux Kernel Bug Hunting
- Online: [Linux Kernel Bug Hunting](https://docs.kernel.org/admin-guide/bug-hunting.html)
- 本章阅读定位：本章主官方资料：如何使用 Oops、symbols、gdb/objdump 定位。

### ALIENTEK Linux Driver Guide V1.5.2
- Local expected path: `../references/ALIENTEK_iMX6ULL_Linux_Driver_Guide_V1.5.2.pdf`
- Online: [ALIENTEK Linux Driver Guide V1.5.2](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- 本章阅读定位：若手册包含 Kernel 异常/驱动调试章节可对照；Oops 精确方法优先官方 Kernel docs。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch24_module_symbols_vermagic.md) · [Next →](ch26_zephyr_scheduling.md)
