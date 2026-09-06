# Chapter 10 - 编译 Kernel、DTB、modules

## 10.1 进入 Kernel root

```bash
cd "$KERNEL_SRC"
pwd
ls
```

应该看到：

```text
Makefile
arch/
drivers/
fs/
include/
kernel/
scripts/
```

确认：

```bash
make kernelversion
grep -E '^VERSION|^PATCHLEVEL|^SUBLEVEL|^EXTRAVERSION' Makefile
```

---

## 10.2 打开 vendor `build.sh`

```bash
nl -ba build.sh | sed -n '1,300p'
```

只摘：

```text
ARCH =
CROSS_COMPILE =
defconfig =
zImage target =
dtb target =
modules target =
output tmp/ =
```

---

## 10.3 验证 toolchain

```bash
grep -nE 'CROSS_COMPILE|arm-poky|arm-linux' build.sh
```

按结果检查：

```bash
which arm-poky-linux-gnueabi-gcc || true
which arm-linux-gnueabihf-gcc || true
```

---

## 10.4 完整编译

```bash
mkdir -p ~/work/linux/bsp/imx6ull/build_logs

cd "$KERNEL_SRC"

./build.sh 2>&1 | tee   ~/work/linux/bsp/imx6ull/build_logs/kernel_full_build_$(date +%Y%m%d_%H%M%S).log
```

结束：

```bash
echo "${PIPESTATUS[0]}"
```

必须为 `0`。

---

## 10.5 找 `zImage`

```bash
find tmp -type f -name zImage -ls 2>/dev/null
ls -lh arch/arm/boot/zImage
```

保存属性：

```bash
stat arch/arm/boot/zImage
sha256sum arch/arm/boot/zImage
```

---

## 10.6 找 ALIENTEK DTB

```bash
find arch/arm/boot/dts -maxdepth 1   -name 'imx6ull*alientek*.dtb'   -printf '%f
' | sort
```

常见：

```text
imx6ull-alientek-emmc.dtb
```

确认：

```bash
ls -lh arch/arm/boot/dts/imx6ull-alientek-emmc.dtb
```

不存在则：

```bash
find tmp -type f -name '*.dtb' | sort | grep -i alientek
```

---

## 10.7 反编译 DTB 做 sanity check

```bash
dtc -I dtb -O dts   arch/arm/boot/dts/imx6ull-alientek-emmc.dtb   -o /tmp/week2-imx6ull.dts
```

看：

```bash
grep -nE 'model =|compatible ='   /tmp/week2-imx6ull.dts | head -20
```

再：

```bash
grep -nE 'chosen|stdout-path|serial'   /tmp/week2-imx6ull.dts | head -50
```

只确认：
- DTB 可解析；
- model/compatible 合理；
- 不是拿错 EVK DTB。

---

## 10.8 找 modules

正点原子 4.4 说明 vendor `tmp/` 中会有：

```text
modules.tar.bz2
```

检查：

```bash
find tmp -maxdepth 2 -type f   \( -name 'modules*.tar*' -o -name '*.ko' \)   -ls 2>/dev/null
```

若有：

```bash
tar tjf tmp/modules.tar.bz2 | head -50
```

---

## 10.9 保存 artifact

```bash
ART=~/work/linux/bsp/imx6ull/artifacts/kernel
mkdir -p "$ART"

cp -av arch/arm/boot/zImage "$ART/"
cp -av   arch/arm/boot/dts/imx6ull-alientek-emmc.dtb   "$ART/"

cp -av tmp/modules.tar.bz2 "$ART/"   2>/dev/null || true
```

再：

```bash
cd "$ART"
sha256sum * | tee SHA256SUMS.txt
```

---

## 10.10 保存 Kernel `.config`

```bash
cp "$KERNEL_SRC/.config"   "$ART/kernel_dot_config"
```

查：

```bash
grep -E   '^CONFIG_LOCALVERSION|^CONFIG_MODULES|^CONFIG_OF|^CONFIG_FEC'   "$KERNEL_SRC/.config"   | tee "$ART/kernel_config_summary.txt"
```

---

## 10.11 给本周 Kernel 加标记

为了 Day 4 启动后能确认新 kernel，推荐：

```bash
make ARCH=arm menuconfig
```

进入：

```text
General setup
  -> Local version - append to kernel release
```

填：

```text
-week2
```

重新执行 vendor build。

最终启动后：

```bash
uname -r
```

应包含 `-week2`。

如果不想改配置，至少要用 boot log 的 build timestamp/hash 证明。

---

## 10.12 只改 DTB 时

后面只改 DTS：

```bash
make ARCH=arm   CROSS_COMPILE=<actual-prefix>   dtbs -j$(nproc)
```

如果 vendor build 使用特殊 `O=` 或环境，以 `build.sh` 为准。

---

## 10.13 常见失败

### DTB target 不存在

```bash
grep -R   'imx6ull-alientek-emmc.dtb'   -n arch/arm/boot/dts
```

### 找不到输出

```bash
find . -type f -newermt '20 minutes ago'   \( -name zImage -o -name '*.dtb' -o -name 'modules*.tar*' \)   -ls
```

### Build 失败

```bash
tail -150   ~/work/linux/bsp/imx6ull/build_logs/kernel_full_build_*.log
```

从第一个 error 向上看。

---

## 10.14 精确确认 Kernel release

构建完成：

```bash
cd "$KERNEL_SRC"

make -s kernelrelease
```

如果 vendor build 使用 CROSS_COMPILE/ARCH 环境，按实际脚本带上：

```bash
make ARCH=arm \
  CROSS_COMPILE=<actual-prefix> \
  -s kernelrelease
```

保存：

```bash
make ARCH=arm \
  CROSS_COMPILE=<actual-prefix> \
  -s kernelrelease \
  | tee \
  ~/work/linux/bsp/imx6ull/artifacts/kernel/kernelrelease.txt
```

modules 安装目录必须与这个 release 对得上。

---

## 10.15 解包 modules，检查 `.ko`

```bash
mkdir -p /tmp/week2_modules
rm -rf /tmp/week2_modules/*

tar xjf \
  ~/work/linux/bsp/imx6ull/artifacts/kernel/modules.tar.bz2 \
  -C /tmp/week2_modules
```

如果文件名不同，以真实 package 为准。

查：

```bash
find /tmp/week2_modules \
  -type f -name '*.ko' \
  | head -50
```

统计：

```bash
find /tmp/week2_modules \
  -type f -name '*.ko' \
  | wc -l
```

任选一个：

```bash
modinfo \
  $(find /tmp/week2_modules -type f -name '*.ko' | head -1) \
  2>/dev/null || true
```

Host 的 `modinfo` 不一定能正确解析所有旧 ARM module dependency，但至少可看 ELF/metadata。

---

## 10.16 检查 module ELF 架构

```bash
KO=$(find /tmp/week2_modules \
  -type f -name '*.ko' \
  | head -1)

file "$KO"
```

应该是 ARM relocatable ELF。

再：

```bash
readelf -h "$KO" | \
  grep -E 'Class:|Type:|Machine:'
```

---

## 10.17 记录 DTB 大小和 hash

```bash
cd ~/work/linux/bsp/imx6ull/artifacts/kernel

stat -c '%n %s bytes' \
  imx6ull-alientek-emmc.dtb \
  | tee dtb_size.txt

sha256sum \
  imx6ull-alientek-emmc.dtb
```

以后只改 DTS 时：
- DTB size/hash 应变化；
- zImage 可以不变。

这可以快速判断“我改的 DTS 是否真的编进了目标 DTB”。

---

## 10.18 做一次无功能 DTS rebuild 验证

不修改 DTS：

```bash
cd "$KERNEL_SRC"

make ARCH=arm \
  CROSS_COMPILE=<actual-prefix> \
  dtbs -j$(nproc)
```

然后：

```bash
stat arch/arm/boot/dts/imx6ull-alientek-emmc.dtb
```

如果 vendor kernel 要先 source 环境或 build.sh 传额外参数，就把命令改成脚本真实值。

---

## 10.19 保存构建时间和 Host 信息

```bash
{
  date -Is
  hostname
  uname -a
  make -s kernelrelease 2>/dev/null || true
  sha256sum arch/arm/boot/zImage
  sha256sum arch/arm/boot/dts/imx6ull-alientek-emmc.dtb
} | tee \
  ~/work/linux/bsp/imx6ull/artifacts/kernel/build_manifest.txt
```

Day 4 TFTP 启动成功后，这就是“Host build evidence”。

## 10.14 Day 3 验收

必须：

```text
artifacts/kernel/zImage
artifacts/kernel/imx6ull-alientek-emmc.dtb
artifacts/kernel/kernel_dot_config
artifacts/kernel/SHA256SUMS.txt
```

modules 必须明确记录：
- 是否生成；
- 生成路径；
- 打包格式。

---

## 10.15 资料

- 正点原子快速体验 **4.4 编译出厂源码内核及模块**
- 官方产物：`zImage`、DTB、`modules.tar.bz2`
- [4.4 Online](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/comple_core/)
