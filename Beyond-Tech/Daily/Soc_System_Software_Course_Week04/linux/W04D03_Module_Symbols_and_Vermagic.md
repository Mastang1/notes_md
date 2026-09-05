# W04D03 - Module Symbols and Vermagic: why invalid module format happens

## 0. 今日定位

- 主线：Kernel module ABI/debug
- 时间：2h
- 平台：6ULL + host build tree
- 产物：`module_symbol_map.md` + one controlled mismatch log

## 1. 今天解决的工程问题

驱动移植里非常常见：`.ko` 编译成功，但目标机 `insmod` 报 `invalid module format` 或 unknown symbol。今天把错误拆成版本/配置/符号三类。

## 2. 今日能力构成

```mermaid
flowchart LR
    KO[.ko] --> VM[vermagic]
    KO --> UND[undefined required symbols]
    KO --> EXP[module exports]
    K[Running kernel] --> KS[/proc/kallsyms]
    K --> SYM[exported symbol table]
    VM --> LOAD[module loader]
    UND --> LOAD
    SYM --> LOAD
```

## 3. 先理解：费曼解释

### 3.1 30 秒白话模型

模块像一个要插进“正在运行内核”的插件：除了 CPU 架构对，还要接口/版本足够匹配，并且它依赖的 kernel symbol 必须能被解析。

### 3.2 精确工程模型

`vermagic` 编码 kernel release 和若干 build characteristics；`CONFIG_MODVERSIONS` 场景还会涉及 symbol CRC。模块的 undefined symbols 由 kernel/module dependency 提供。`EXPORT_SYMBOL`/`EXPORT_SYMBOL_GPL` 是内核代码向模块暴露符号的主要机制。

### 3.3 今天必须避免的误解

- API 名字背下来不等于理解执行路径。
- 看到一次成功输出不等于建立了可复现工程闭环。
- 教程里的地址/路径只能作为例子；板上真实值必须用工具验证。

## 4. 原理与执行路径

用工具看证据：`modinfo -F vermagic`、`uname -r`、`nm -u module.ko`、`/proc/kallsyms`、`dmesg`。不要只凭“kernel 都是 4.1.15”判断兼容。

## 5. UML / 时序

本日核心问题主要是静态结构，不强行画时序图。

## 6. References / Manuals

- [ALIENTEK Driver Guide V1.5.2](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf) — Ch.40/41/42 module build/load examples; search `modprobe`, `depmod`, `obj-m`.
- [Linux external module documentation](https://docs.kernel.org/kbuild/modules.html) — sections on `Module.symvers`, symbol versioning, symbols from another module.
- Runtime evidence: `modinfo`, `/proc/kallsyms`, `dmesg`.

## 7. 实验准备

Use yesterday `hello_course.ko` known-good baseline. Save a copy before mismatch experiment.

## 8. 实验

### Lab A - symbols
```bash
modinfo -F vermagic hello_course.ko
nm -u hello_course.ko | head -30
nm hello_course.ko | grep -E 'hello_init|hello_exit'
# target
uname -r
grep -w 'printk' /proc/kallsyms | head || true
```

### Lab B - controlled mismatch
Preferred method: build the same module against a **different prepared kernel tree/config** already available, copy it under a different filename, and attempt `insmod`. Do not corrupt the known-good `.ko` binary manually.

Capture:
```bash
insmod ./hello_mismatch.ko 2>&1 | tee mismatch_user.log
dmesg | tail -50 | tee mismatch_dmesg.log
```

Classify the exact reason from dmesg. If you do not have a second kernel build, do not fabricate a mismatch; document the evidence required and move on.

## 9. 故障注入

- Build one helper module that exports a symbol and another that consumes it; try loading consumer first to observe unknown symbol. Then load provider → consumer. This is optional if time remains.

## 10. 调试路径

`insmod` message → dmesg exact reason → vermagic → `nm -u` → dependency module → `/proc/kallsyms` / Module.symvers. This order is reusable for vendor driver ports.

## 11. 源码 / 系统对象追踪

Read generated `Module.symvers` only. Search kernel source for one exported symbol used by your module and see `EXPORT_SYMBOL*` call.

## 12. 今日验收

- [ ] can read module vermagic.
- [ ] can identify undefined symbols.
- [ ] one mismatch/unknown-symbol scenario is captured or explicitly documented as unavailable.
- [ ] can explain why successful compilation does not guarantee loadability.

## 13. 面试式复述

1. `invalid module format` 常见原因？
2. unknown symbol 如何定位？
3. Module.symvers 干什么？
4. EXPORT_SYMBOL_GPL 与 EXPORT_SYMBOL 区别是什么层面的限制？
5. 为什么 Yocto 外置 module 特别容易出现版本对齐问题？

## 14. Git 交付物

`module_symbol_map.md`, mismatch logs; commit `lab: diagnose module symbols and vermagic`

## 15. 明日连接

Tomorrow practice a minimal kernel fault and map the crash address back to source.
