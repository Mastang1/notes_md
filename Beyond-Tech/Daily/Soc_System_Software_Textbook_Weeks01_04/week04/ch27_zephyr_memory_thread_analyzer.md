# Chapter 27 - RTOS Memory Is a Budget: Stack, Heap and Thread Analyzer

> Week 4 / Day 6 - 把 RTOS 内存从“够不够用”升级为可量化资源预算。

[← Part README](README.md) · [← Previous](ch26_zephyr_scheduling.md) · [Next →](ch28_week4_integration.md)

## 27.1 RTOS 中“有多少 RAM”不够，必须回答“每个 thread 最坏要多少 stack”

Linux user process 可以有 VM/page backing；MCU RTOS RAM 固定而紧张，线程 stack 往往在 build/boot 时就预留。一个 thread stack 配太大浪费产品 RAM，配太小则可能静默破坏相邻内存。

费曼模型：RTOS stack budget 像飞机配载。只知道总载重够不够没用，每个舱位超限都可能出事故。

## 27.2 先画 STM32F407 的 RAM 消费者

```text
.data/.bss
kernel objects
thread stacks
system/workqueue stacks
heap(s)
driver buffers
network/filesystem buffers (后续)
interrupt/exception stack semantics
```

今天不追 exact linker map 全部细节，但要用 build output/map 证明主要静态区域。

## 27.3 Thread stack 为什么不能用“函数局部变量大小相加”粗算

实际需要考虑：

- deepest call chain；
- compiler register spill/alignment；
- printf/logging/formatting；
- interrupts/context frames（取决于 architecture/stack model）；
- library calls；
- debug vs optimized build 差异。

所以必须“设计估算 + runtime measurement + margin”。

## 27.4 开启 Thread Analyzer

按当前 Zephyr 配置启用 thread analyzer，常见相关 config 以官方文档为准。运行后周期/主动输出各 thread：

```text
name
stack size
used
usage percentage
```

不要直接把 analyzer 数字当 worst-case；它只覆盖你实际跑到的路径。

## 27.5 Worked Example：给 Chapter 26 三线程建立 stack budget

先做 baseline：

| Thread | Configured | Observed peak | Margin | Decision |
|---|---:|---:|---:|---|
| producer | | | | |
| worker | | | | |
| logger | | | | |

然后给 worker 增加一段局部数组/更深函数链（受控实验），再测使用量变化。

## 27.6 故障注入：故意把一个测试 thread stack 缩得过小

只在实验 app。启用可用的 stack sentinel/overflow detection/MPU guard（取决于 config/architecture），观察：

- build-time warning；
- runtime fatal error；
- thread analyzer 高水位；
- debugger fault state。

恢复后重新 clean build。

目标：你以后看到“偶发 HardFault”会把 stack overflow 纳入系统化排查，而不是只看业务逻辑。

## 27.7 Heap：Zephyr 不等于只能静态分配

Zephyr 有 kernel heap/system heap/malloc compatibility 等机制，具体取决于 config。对产品设计而言，更重要的是：

```text
谁分配？
生命周期？
失败怎么办？
碎片风险？
能否静态预算？
```

本课程暂不做通用 allocator 深挖。你的下一份 System Software 岗更需要能做 resource accounting，而不是背 `k_malloc`。

## 27.8 Linker map 与 runtime analyzer 要一起看

Build 后保存 `.map`/size：

```bash
west build -t rom_report  # 若当前 Zephyr 版本/target 支持
west build -t ram_report
```

或使用 Zephyr/SDK 当前提供的 size/report target。配合：

```bash
arm-zephyr-eabi-size build/zephyr/zephyr.elf
```

静态报告回答“链接后占多少”；thread analyzer 回答“运行时 stack 用到多少”。两者不能互相替代。

## 27.9 Independent Challenge：给你的未来 MCUboot 产品提前设资源门槛

写 `resource_budget.md`：

```text
Application Flash max
Static RAM max
Per-thread stack margin policy
Heap policy
Logging debug/release difference
```

现在数值可以基于现有 app baseline，后面每加 shell/MCUmgr/MCUboot 就比较回归。

## 27.10 下一章：Week 4 最后把 Linux process/thread 与 Zephyr thread/context 放到同一维度比较

不是为了列“Linux 重、RTOS 轻”，而是比较 scheduler unit、address space、blocking、memory protection、driver execution context、debug 方法。这会成为后面 IRQ/concurrency 学习的共同坐标系。

## References and manuals

### Zephyr Thread Analyzer
- Online: [Zephyr Thread Analyzer](https://docs.zephyrproject.org/latest/services/debugging/thread-analyzer.html)
- 本章阅读定位：本章主资料：stack usage/analyzer 配置与输出。

### Zephyr Memory Management
- Online: [Zephyr Memory Management](https://docs.zephyrproject.org/latest/kernel/memory_management/index.html)
- 本章阅读定位：只读 heap/memory services 总览。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch26_zephyr_scheduling.md) · [Next →](ch28_week4_integration.md)
