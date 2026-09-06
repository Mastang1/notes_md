# Chapter 23 - Your First Kernel Module: Build, Load, Execute and Unload

> Week 4 / Day 2 - 用最小 `.ko` 观察 module 在运行 Kernel 中的生命周期。

[← Part README](README.md) · [← Previous](ch22_kernel_build_kconfig_kbuild.md) · [Next →](ch24_module_symbols_vermagic.md)

## 23.1 第一个 Module 的价值：观察“动态加入 Kernel”这件事本身

这章不要塞 char device、DTS、IRQ。只保留 module lifecycle，确保每个现象都能解释。

## 23.2 最小 module 源码

`hello_mod.c`：

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "number of log lines");

static int __init hello_init(void)
{
    int i;
    pr_info("hello_mod: init count=%d\n", count);
    for (i = 0; i < count; ++i)
        pr_info("hello_mod: line=%d\n", i);
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("hello_mod: exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("course");
MODULE_DESCRIPTION("minimal module lifecycle lab");
```

## 23.3 Out-of-tree Kbuild 文件

`Makefile`：

```make
obj-m += hello_mod.o

KDIR ?= /path/to/kernel/build
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
```

`-C $(KDIR)` 让 make 进入 Kernel build system；`M=$(CURDIR)` 告诉 Kernel 外部 module 源码在哪里。这不是普通 gcc 手工链接任务。

## 23.4 为什么 Module 必须针对“运行中的那个 Kernel”构建

至少需要匹配：

- architecture；
- Kernel configuration；
- generated headers；
- module ABI/versioning；
- toolchain/feature 兼容性。

所以 `KDIR` 不应该随便指向一个同版本 tarball，而应指向与目标 Kernel 构建对应的 source/build tree。

## 23.5 Worked Example：Host 生成 `.ko` 后先分析，不急着 insmod

```bash
make
file hello_mod.ko
modinfo hello_mod.ko
arm-linux-gnueabihf-nm hello_mod.ko | head -40
readelf -h hello_mod.ko | head -25
```

`.ko` 本质仍是 ELF relocatable/module object，里面有 metadata sections、symbol references、code/data。

## 23.6 Target Lifecycle：装载、观察、卸载

把 module 放 NFS：

```bash
cp hello_mod.ko ~/nfs/
```

板端：

```bash
dmesg -w
insmod /mnt/nfs/hello_mod.ko count=3
lsmod | grep hello_mod
modinfo /mnt/nfs/hello_mod.ko
rmmod hello_mod
```

另一个窗口：

```bash
ls -l /sys/module/hello_mod
find /sys/module/hello_mod/parameters -maxdepth 1 -type f -print
cat /sys/module/hello_mod/parameters/count
```

这让 module 从“代码”变成 sysfs/runtime object。

## 23.7 `module_init` 不是程序 main

module 已经进入一个**正在运行的 Kernel**。loader 完成映射/relocation 后调用 init callback。它没有自己的 process address space，也不是独立进程。

费曼对比：

```text
User app: exec -> new process image -> main
Kernel module: load into kernel -> resolve symbols -> init callback
```

## 23.8 Guided Lab：参数修改与权限

`module_param(count, int, 0644)` 产生 sysfs parameter。尝试：

```bash
echo 5 > /sys/module/hello_mod/parameters/count
cat /sys/module/hello_mod/parameters/count
```

注意：参数在 init 后修改不会自动让 init 再执行。parameter 是状态，init 是生命周期事件。

## 23.9 Independent Challenge：连续加载/卸载 20 次并检查污染

写 shell loop，确保每次：

- insmod 成功；
- rmmod 成功；
- dmesg 有成对 init/exit；
- `/sys/module/hello_mod` 最终不存在。

这建立以后 Driver “probe/remove 对称性”的习惯。

## 23.10 下一章：Module 能装入并不神秘，真正关键是它怎样使用 Kernel symbols、怎样检查 ABI

Chapter 24 专门拆 `undefined symbol`、`EXPORT_SYMBOL`、module dependency、vermagic。以后 `invalid module format` 不再靠重新编一遍碰运气。

## References and manuals

### ALIENTEK Linux Driver Guide V1.5.2
- Local expected path: `../references/ALIENTEK_iMX6ULL_Linux_Driver_Guide_V1.5.2.pdf`
- Online: [ALIENTEK Linux Driver Guide V1.5.2](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- 本章阅读定位：重点找 Linux 驱动模块、模块加载卸载、模块参数相关章节。

### Building External Modules
- Online: [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- 本章阅读定位：本章主官方资料：`make -C ... M=... modules` 及 module build requirements。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch22_kernel_build_kconfig_kbuild.md) · [Next →](ch24_module_symbols_vermagic.md)
