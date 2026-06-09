## 1. build command

```shell
#!/bin/sh

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- distclean

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- imx_alientek_emmc_defconfig

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- all HOSTCFLAGS="-fcommon" -j16
```

```shell
#build a kernel module
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

```make
KERNELDIR := /data/share/public/imx6ull/kernel_uboot_src/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek

CURRENT_PATH := $(shell pwd)

obj-m := chrdevbase.o

build: kernel_modules

kernel_modules:

    $(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:

    $(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
```

## 2. 本质理解

**本质总结：** `ARCH` 决定了**输入什么材料（源码路径）**，`CROSS_COMPILE` 决定了**使用什么加工机器（工具链）**。两者在逻辑上是完全解耦的，缺一不可。

### 还需要别的参数吗？

对于你当前**编译外树（Out-of-tree）模块**的任务来说，**不需要任何别的环境参数了。**

你 Makefile 里的： `$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules` 配合终端里的： `make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-`

这四个关键元素已经构成了一个完备的闭环：

1. **`-C $(KERNELDIR)`**：告诉项目经理（Kbuild）主营地在哪里，去那里读取内核的 `.config`（内核的顶层配置）。
    
2. **`M=$(CURRENT_PATH)`**：告诉项目经理，加工完内核自己的基础配置后，再回到当前目录来编译模块。
    
3. **`ARCH=arm`**：锁定内核源码树中的 ARM 硬件材料库。
    
4. **`CROSS_COMPILE=...`**：锁定最终输出机器码的物理加工厂。

## 3. build module文件modinfo架构版本不对
#### 关闭多平台选项
打开图形化配置界面：

Bash

```
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig
```

在弹出的蓝底灰字界面中，通过键盘上下键和回车键，依次进入：

1. 选择 **`System Type --->`** 并回车。
    
2. 选择 **`Multiple platform selection --->`** 并回车。
    
3. 你会看到列表里有 `ARMv6 based platforms (ARM11)` 和 `ARMv7 based platforms (Cortex-A, PRCM)`。
    
4. 将光标移动到 **`ARMv6 based platforms (ARM11)`** 上，**按键盘的空格键**，取消它前面的星号 `[*]`，变成 `[ ]`。
    
5. 确保 **`ARMv7 based platforms (Cortex-A, PRCM)`** 前面依然是 `[*]`。
    
6. 使用键盘左右键选择底部的 `< Save >` 保存，然后 `< Exit >` 一路退出配置界面。