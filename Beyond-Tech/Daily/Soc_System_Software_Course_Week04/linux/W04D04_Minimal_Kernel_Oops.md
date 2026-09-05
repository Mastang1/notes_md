# W04D04 - Minimal Kernel Oops: log -> address -> source

## 0. 今日定位

- 主线：Kernel debugging foundation
- 时间：2h
- 安全：优先 QEMU/可恢复测试板；绝不在公司生产/关键环境执行
- 产物：`oops_basic.md` + captured serial log

## 1. 今天解决的工程问题

你未来做 Driver 必须能读 Oops。今天不追 crash dump 全体系，只训练：发生 kernel fault 后，如何保留串口日志、识别 PC/LR/call trace，并用未 strip ELF/module + addr2line/objdump 回到源代码。

## 2. 今日能力构成

```mermaid
flowchart LR
    BUG[bad kernel access] --> EXC[CPU exception]
    EXC --> OOPS[Kernel Oops log]
    OOPS --> PC[PC/LR/call trace]
    PC --> ELF[vmlinux/module ELF]
    ELF --> A2L[addr2line/objdump]
    A2L --> SRC[source line]
```

## 3. 先理解：费曼解释

### 3.1 30 秒白话模型

Oops 类似 MCU HardFault + 更丰富的内核上下文。不要先“猜原因”，先保住日志，再用地址和符号证据定位。

### 3.2 精确工程模型

ARM kernel page fault会进入异常处理并打印寄存器/调用栈。模块地址可能需要结合 module load address/symbol info 才能转换；具体日志格式随 kernel version 不同。`addr2line` 只对与当前二进制匹配的符号地址有意义。

### 3.3 今天必须避免的误解

- API 名字背下来不等于理解执行路径。
- 看到一次成功输出不等于建立了可复现工程闭环。
- 教程里的地址/路径只能作为例子；板上真实值必须用工具验证。

## 4. 原理与执行路径

本日分两级：A) `WARN_ON(1)` 安全预演日志阅读；B) 真 NULL dereference 仅在 QEMU/可恢复板上做，串口日志必须开启。

## 5. UML / 时序

本日核心问题主要是静态结构，不强行画时序图。

## 6. References / Manuals

- **ALIENTEK Driver Guide V1.5.2**: [online](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf) — today mainly reuses module build procedure from Ch.40; vendor guide is not the primary crash-debug reference.
- Linux kernel source + your exact `vmlinux`/`.ko` are the primary symbols.
- [GNU GDB manual](https://sourceware.org/gdb/current/onlinedocs/gdb.html) for symbol/address inspection.

Do not use a random vmlinux from another build to decode the crash.

## 7. 实验准备

Ensure serial console logging works. Keep yesterday module build known-good. If using physical 6ULL, boot from a replaceable dev setup (TFTP/NFS preferred) and be ready to power cycle.

## 8. 实验

### Stage A - WARN_ON safe pass
```c
static int __init oops_init(void) {
    pr_info("oops_course: before warn\n");
    WARN_ON(1);
    pr_info("oops_course: after warn\n");
    return 0;
}
```
Load and capture the complete warning/call trace.

### Stage B - NULL dereference (QEMU/recoverable target only)
```c
static int __init oops_init(void) {
    volatile int *p = NULL;
    pr_info("oops_course: triggering null dereference\n");
    *p = 0x1234;
    return 0;
}
```

Never run Stage B on a machine where downtime/data loss matters.

### Decode
```bash
# exact command depends on whether log address is module-relative/absolute
arm-linux-gnueabihf-objdump -S oops_course.ko | less
arm-linux-gnueabihf-nm -n oops_course.ko
arm-linux-gnueabihf-addr2line -e oops_course.ko <appropriate-offset>
```

In `oops_basic.md` write how you derived the address/offset; do not paste an arbitrary addr2line command without explaining it.

## 9. 故障注入

- Use a module binary from a different build with addr2line and show why source mapping becomes meaningless.
- Truncate the serial log and identify which critical fields you lost; this motivates reliable log capture.

## 10. 调试路径

serial log → fault type → PC/LR/call trace → module/function identity → exact matching ELF → objdump/nm/addr2line → source. Later weeks add ftrace/kgdb/crash/KASAN.

## 11. 源码 / 系统对象追踪

Look at the source around the resolved function only. Do not recursively read ARM fault handling yet.

## 12. 今日验收

- [ ] WARN_ON trace captured and understood.
- [ ] NULL fault performed only in safe environment or explicitly skipped with reason.
- [ ] one address mapped back to function/source using matching binary.
- [ ] can explain why symbols/build identity matter.

## 13. 面试式复述

1. Oops 和 panic 一定相同吗？
2. 为什么串口日志优先？
3. PC/LR/call trace 各提供什么？
4. addr2line 为什么必须匹配 binary？
5. 模块 crash 与 built-in kernel crash 的地址解析有何额外步骤？

## 14. Git 交付物

`oops_basic.md`, full serial log, module source; commit `lab: capture and decode a controlled kernel warning/oops`

## 15. 明日连接

Day5 回到 Zephyr，深入 scheduler/synchronization；比较“kernel context”在 Linux 与 RTOS 中的不同。
