# Chapter 20 - Stop Debugging with printf: west debug, GDB and Target State

> Week 3 / Day 6 - 把已有 JTAG 经验迁移到 Zephyr runner/GDB 工具链。

[← Part README](README.md) · [← Previous](ch19_zephyr_led_key_devicetree.md) · [Next →](ch21_week3_integration.md)

## 20.1 你会 JTAG，不等于你已经会调 Zephyr：缺的是“工具链路”映射

你过去用 Lauterbach/PE/JTAG，底层经验足够。Zephyr 新东西是：

```text
west debug command
 -> board runner
 -> OpenOCD/J-Link server
 -> GDB Remote Protocol
 -> SWD/JTAG probe
 -> Cortex-M target
```

所以今天不是教 `break main`，而是搞清每层出了问题应该查哪里。

## 20.2 `west debug` 与 `west attach` 的区别

通常：

- debug：按 runner 配置启动 debug server 并连接 GDB，可能含 reset/load 语义；
- attach：连接到已运行 target，更强调不重刷/少扰动。

具体 runner 行为以当前 Zephyr/board 文档为准。先用：

```bash
west flash --context
west debug --context
```

查看选中的 runner/参数。

## 20.3 Worked Example：在 `main()` 停住并证明 ELF/target 一致

先 clean build：

```bash
west build -p always -b f407_explorer <smoke-app> -- -DBOARD_ROOT=<root>
```

确认 ELF：

```bash
arm-zephyr-eabi-readelf -h build/zephyr/zephyr.elf
arm-zephyr-eabi-nm -n build/zephyr/zephyr.elf | grep ' main$'
```

进入 debug：

```bash
west debug
```

GDB：

```gdb
break main
continue
bt
info registers
x/16wx $sp
```

## 20.4 Thread awareness：为什么普通 bare-metal GDB 只能看到 CPU，而 RTOS-aware debug 能看到任务

Zephyr thread 是 kernel data structure。单纯 CPU GDB 能看当前 PC/SP/register；要列所有 thread，需要 RTOS awareness、GDB helper、OpenOCD/J-Link plugin 或 Zephyr debug tooling 的支持。

今天最低验收：

- 能停当前 thread；
- 能看 stack/register；
- 能从 ELF symbol 到 source line；
- 能区分“目标硬件没连上”和“符号没加载对”。

Week 4 再系统看 scheduler/thread analyzer。

## 20.5 Guided Lab：断在 KEY 事件路径

在 Chapter 19 的 key handling function 设断点：

1. 程序正常运行；
2. 按键；
3. 进入断点；
4. 查看 GPIO logical value；
5. single-step 到 LED toggle；
6. 继续运行。

这比断在 main 更接近真实 Driver/Application debug。

## 20.6 Fault 1：GDB ELF 用错

重新 build 但不烧录，或烧录 A 却 GDB 载入 B，观察 breakpoint 地址/源码错位。恢复同一 `zephyr.elf`。

和 Linux remote GDB 完全同一纪律：**debug symbol 必须对应 target binary。**

## 20.7 Fault 2：runner/probe 层故障

拔掉 probe 或故意选错 runner。看错误来自：

```text
west/CMake selection
runner executable
USB/probe
SWD/JTAG transport
target halt/reset
```

以后不要把 “Cannot connect to target” 当成 Zephyr app bug。

## 20.8 Independent Challenge：不用 west，说明它底层帮你做了什么

通过 `west debug --context` 和进程列表/runner 文档写出等价链：debug server 如何启动、GDB 连接哪个端口、使用哪个 ELF。

目标不是以后手工起 OpenOCD，而是知道自动化层下面有什么。

## 20.9 下一章：Week 3 的两条线已经各自到达“系统对象”

Linux：process -> loader -> VA -> fd/struct file。Zephyr：DTS -> device -> debug。Chapter 21 把这些对象重新从记忆中画出来，检查哪些只是“看懂了”但还不能脱稿解释。

## References and manuals

### Zephyr Debugging
- Online: [Zephyr Debugging](https://docs.zephyrproject.org/latest/develop/debug/index.html)
- 本章阅读定位：看 west debug/runner/GDB 基本路径。

### GDB Remote Debugging
- Online: [GDB Remote Debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)
- 本章阅读定位：把 Zephyr debug server 与标准 GDB Remote Protocol 对上。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch19_zephyr_led_key_devicetree.md) · [Next →](ch21_week3_integration.md)
