# Chapter 21 - Integration: Reconstruct User Space and Zephyr Device Models from Memory

> Week 3 / Day 7 - 用三张图和一次口述把 process/memory/fd/device 变成长期心智模型。

[← Part README](README.md) · [← Previous](ch20_zephyr_west_debug.md)

## 21.1 本章不增加新 API：用“重建模型”判断前三周有没有学实

你最容易产生的错觉是：看文档时每句话都懂，关掉文档就无法从系统问题推出对象关系。今天做 retrieval，不做抄写。

## 21.2 第一张白纸：从 shell command 到 syscall

题目：执行 `/bin/echo hello` 后，画出至少：

```text
shell process
 -> fork/clone
 -> child execve
 -> Kernel ELF loader
 -> dynamic linker
 -> main
 -> write syscall
 -> fd=1
 -> struct file
 -> terminal/pipe implementation
```

先画，再打开 Ch15/16/18 校正。

## 21.3 第二张白纸：从一个指针到实际内存

画：

```text
User pointer/VA
 -> VMA permission
 -> TLB
 -> page table
 -> PA
 -> cache/memory
```

然后回答：

- fork 后相同 VA 是否一定相同 PA？
- malloc 返回的是 PA 吗？
- `/proc/pid/maps` 是 page table dump 吗？

## 21.4 第三张白纸：从 F407 原理图 LED0 到 C API

必须包含：

```text
schematic PF9 active-low
 -> board DTS gpio-leds
 -> binding
 -> final zephyr.dts/generated metadata
 -> DT_ALIAS / gpio_dt_spec
 -> gpio API
 -> STM32 GPIO driver
 -> register/pin
```

任何一步画不出，就回到 Chapter 19 补。

## 21.5 Linux 与 Zephyr 的“对象统一”不是一回事

### Linux

VFS/Driver Model 依赖 runtime kernel objects，process/syscall 会动态打开/引用对象。

### Zephyr

资源更静态，很多 device/DT 信息在 build-time 已生成。不是“Zephyr 就是小 Linux”。两者都追求 abstraction/driver reuse，但生命周期和动态性不同。

## 21.6 Guided Lab：10 分钟口述，限制自己只能画 3 张图

建议录音：

1. 3 分钟：process/exec/ELF；
2. 3 分钟：VA/page；
3. 4 分钟：fd/VFS + Zephyr device。

录完再查文档，记录“说错/说不出来”的 5 个点。

## 21.7 Week 3 Gate

- [ ] 能用 strace 解释 fork/exec/wait；
- [ ] 能从 Program Header 解释 ELF loader；
- [ ] 能读 `/proc/pid/maps`；
- [ ] 能解释 fd -> struct file -> operations；
- [ ] F407 LED/KEY 在自定义 board 上工作；
- [ ] 能用 west debug/GDB 观察 target；
- [ ] 三张系统图可脱稿画出。

## 21.8 Part III 结语：现在才进入 Kernel Module

Week 4 的 Linux 主线终于开始 `.ko`。为什么现在才写？因为 module 的价值不是“printk hello”，而是它将成为用户态 fd/UAPI、DeviceTree/Driver Model、IRQ/DMA 等机制的承载体。你已经有上层系统模型，进入 Kernel 不会只剩 API 记忆。

## References and manuals

### Linux man-pages index
- Online: [Linux man-pages index](https://man7.org/linux/man-pages/)
- 本章阅读定位：用于对照 fork/exec/mmap/open/poll 的精确定义。

### Zephyr Devicetree
- Online: [Zephyr Devicetree](https://docs.zephyrproject.org/latest/build/dts/index.html)
- 本章阅读定位：用于校正 board DTS/binding/generated model。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch20_zephyr_west_debug.md)
