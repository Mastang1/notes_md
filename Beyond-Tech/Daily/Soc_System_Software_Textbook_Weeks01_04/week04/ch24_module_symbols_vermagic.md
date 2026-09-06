# Chapter 24 - How a Module Joins a Running Kernel: Symbols, Dependencies and vermagic

> Week 4 / Day 3 - 把 `invalid module format` 和 `unknown symbol` 变成可分类问题。

[← Part README](README.md) · [← Previous](ch23_first_kernel_module.md) · [Next →](ch25_kernel_oops_debug.md)

## 24.1 “模块加载失败”先判断是哪一种失败

典型分类：

```text
file/arch wrong
vermagic/ABI mismatch
unknown symbol
symbol version mismatch
module dependency missing
init function returns error
```

这些不是一回事。今天用工具把它们分开。

## 24.2 `nm` 看 `.ko`：U 符号表示“我需要别人提供”

```bash
${CROSS_COMPILE}nm hello_mod.ko | head -80
```

常见：

```text
U printk/pr_info related symbol
T hello_init local code...
```

`U` = undefined in this object，加载时由 Kernel/module symbol table 解析。

## 24.3 Kernel exports：不是 vmlinux 中所有函数都允许 Module 使用

Kernel source 中：

```c
EXPORT_SYMBOL(symbol);
EXPORT_SYMBOL_GPL(symbol);
```

导出形成可供 module resolver 使用的集合。`/proc/kallsyms` 能看到大量运行时 symbol，但“能看到”与“可给外部 module link/use”不是简单等价。

查：

```bash
grep '<symbol>' /proc/kallsyms
```

构建树还可看 `Module.symvers`（若生成）。

## 24.4 vermagic：一个快速兼容性指纹，不是完整 ABI 证明

```bash
modinfo hello_mod.ko | grep vermagic
uname -r
```

vermagic 常包含 kernel release、SMP/preempt/mod_unload/architecture 等信息。insmod 会检查兼容性。

费曼模型：vermagic 像“外包装规格标签”，能快速挡掉明显不匹配，但模块兼容还可能受 symbol version/config/toolchain/结构变化影响。

## 24.5 Worked Example：故意制造 `invalid module format`

最安全方式：用与你板端不同的 Kernel build tree 构建同一 module，然后尝试加载。立即看：

```bash
dmesg | tail -50
modinfo bad_module.ko | grep vermagic
```

**真正诊断信息常在 dmesg，不只在 insmod 一行。**

恢复正确 build tree。

## 24.6 Module dependency：`insmod` 和 `modprobe` 为什么行为不同

- `insmod path.ko`：直接请求加载这个文件，不帮你完整解析依赖；
- `modprobe name`：依赖 modules.dep 等元数据，自动加载依赖/别名。

在嵌入式裁剪 rootfs 上，modprobe/database 可能不完整，所以实际 BSP 调试常看到直接 insmod；但产品集成时应理解标准 module dependency 机制。

## 24.7 Symbol versioning：为什么名字一样也可能不够

若 `CONFIG_MODVERSIONS` 等机制启用，Kernel 可为 exported symbol ABI 计算/记录 version CRC。module 与 Kernel 对同名 symbol 的 expected version 不一致，也会拒绝。

今天不深挖 genksyms，只知道去哪里看：

```bash
head Module.symvers
modprobe --show-depends <name> 2>/dev/null
```

## 24.8 Guided Lab：让两个自己的 module 建立依赖

`provider.ko`：导出一个简单函数：

```c
int course_add(int a, int b) { return a + b; }
EXPORT_SYMBOL_GPL(course_add);
```

`consumer.ko` 调用 `course_add()`。

实验：

1. 先加载 consumer -> 观察 unknown symbol；
2. 加载 provider -> 再 consumer；
3. consumer 未卸载时尝试 rmmod provider -> 观察 refcount/used by；
4. 正确逆序卸载。

这比死背 EXPORT_SYMBOL 有意义：你亲自看到运行时 dependency。

## 24.9 Independent Challenge：做一张 Module Load Checklist

以后遇到 `.ko` 加载失败，固定顺序：

```text
file -> modinfo/vermagic -> uname -r -> dmesg -> nm undefined -> kallsyms/Module.symvers -> dependency/config
```

把它写进你的 Linux debug playbook。

## 24.10 下一章：前面都是“正常加载”，现在故意让 Kernel 崩一次

Chapter 25 在 QEMU/可恢复环境制造最小 NULL dereference，学习如何从 Oops 中的 PC/Call Trace 回到 vmlinux/module source。目标不是崩系统，而是建立 crash evidence -> source 的路径。

## References and manuals

### ALIENTEK Linux Driver Guide V1.5.2
- Local expected path: `../references/ALIENTEK_iMX6ULL_Linux_Driver_Guide_V1.5.2.pdf`
- Online: [ALIENTEK Linux Driver Guide V1.5.2](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- 本章阅读定位：查模块符号导出、模块依赖/加载相关章节。

### Building External Modules
- Online: [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- 本章阅读定位：重点 Module.symvers、symbol versioning、external module dependencies。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch23_first_kernel_module.md) · [Next →](ch25_kernel_oops_debug.md)
