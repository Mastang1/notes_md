# Chapter 9 - 编译 U-Boot：拿到 `u-boot.imx`

## 9.1 进入 U-Boot root

```bash
cd "$UBOOT_SRC"
pwd
ls
```

应该看到：

```text
Makefile
arch/
board/
cmd/
configs/
drivers/
include/
tools/
```

---

## 9.2 保存 vendor build script

```bash
cp build.sh   ~/work/linux/bsp/imx6ull/notes/uboot_build.sh.reference

nl -ba build.sh | sed -n '1,260p'
```

记录：

```text
defconfig =
cross compiler =
jobs =
output dir =
final image =
```

---

## 9.3 验证编译器

如果脚本用 Poky：

```bash
which arm-poky-linux-gnueabi-gcc
arm-poky-linux-gnueabi-gcc --version
arm-poky-linux-gnueabi-gcc -dumpmachine
```

如果脚本用通用 Linaro：

```bash
which arm-linux-gnueabihf-gcc
arm-linux-gnueabihf-gcc --version
```

以脚本真实要求为准。

---

## 9.4 看脚本是否自动 clean

```bash
grep -nE 'distclean|mrproper|clean' build.sh
```

如果脚本自己 clean，按官方流程直接执行。

如果没有且你要从干净状态构建：

```bash
make distclean
```

不要用 `sudo make`。

---

## 9.5 编译并保存日志

```bash
mkdir -p ~/work/linux/bsp/imx6ull/build_logs

cd "$UBOOT_SRC"

./build.sh 2>&1 | tee   ~/work/linux/bsp/imx6ull/build_logs/uboot_build_$(date +%Y%m%d_%H%M%S).log
```

结束后：

```bash
echo "${PIPESTATUS[0]}"
```

必须是：

```text
0
```

---

## 9.6 找构建产物

正点原子 4.3 说明产物在源码顶层 `tmp/`。

```bash
find . -maxdepth 2 -type f   \( -name 'u-boot.imx' -o -name 'u-boot.bin' -o -name 'u-boot' \)   -printf '%p %s bytes
'
```

再：

```bash
ls -lh tmp 2>/dev/null || true
ls -lh tmp/*u-boot* 2>/dev/null || true
```

必须找到：

```text
u-boot.imx
```

---

## 9.7 检查文件类型

```bash
file u-boot 2>/dev/null || true
file u-boot.bin 2>/dev/null || true
file tmp/u-boot.imx 2>/dev/null || true
```

只记：

```text
u-boot      -> ELF
u-boot.bin  -> binary
u-boot.imx  -> i.MX 启动格式镜像
```

---

## 9.8 保存 artifact

```bash
ART=~/work/linux/bsp/imx6ull/artifacts/uboot
mkdir -p "$ART"

cp -av tmp/u-boot.imx "$ART/"
cp -av u-boot "$ART/" 2>/dev/null || true
```

计算：

```bash
cd "$ART"
sha256sum * | tee SHA256SUMS.txt
```

---

## 9.9 保存 defconfig

找本次实际 defconfig：

```bash
grep -nE 'defconfig' "$UBOOT_SRC/build.sh"
```

然后：

```bash
find "$UBOOT_SRC/configs" -maxdepth 1   -name '*alientek*defconfig' -o -name '*mx6*defconfig'
```

把实际使用的那份复制：

```bash
cp "$UBOOT_SRC/configs/<actual-defconfig>"   "$ART/"
```

---

## 9.10 保存 `.config`

```bash
cp "$UBOOT_SRC/.config" "$ART/uboot_dot_config"
```

记录：

```bash
grep -E   'CONFIG_TARGET|CONFIG_SYS_TEXT_BASE|CONFIG_CMD_TFTP|CONFIG_CMD_PING'   "$UBOOT_SRC/.config" | tee "$ART/config_summary.txt"
```

没有的 symbol 不强求，按当前 U-Boot 版本为准。

---

## 9.11 看 ELF symbol

如果有 `u-boot`：

```bash
nm u-boot 2>/dev/null |   grep -E ' board_init$| main_loop$' | head
```

再：

```bash
readelf -h u-boot 2>/dev/null |   grep -E 'Class:|Machine:|Entry'
```

只是确认 ELF 正常，不做源码分析。

---

## 9.12 今天不烧 U-Boot

不要写 eMMC。

本周 Linux 验证采用：
- 出厂 U-Boot；
- 新 Kernel；
- 新 DTB；
- TFTP 临时启动。

这样 U-Boot 构建错了也不会把板子启动链弄坏。

---

## 9.13 失败分支

### 编译器找不到

```bash
which <required-cross-gcc>
echo "$PATH"
```

回 Day 1 toolchain。

### Build log 失败

```bash
tail -120   ~/work/linux/bsp/imx6ull/build_logs/uboot_build_*.log
```

看第一个真实 `error:`，不要只看最后 `make: *** Error 2`。

### 权限异常

```bash
find "$UBOOT_SRC" -maxdepth 2 ! -user "$USER" -ls | head -50
```

不要 `sudo ./build.sh`。

---

## 9.14 确认本次真正使用的 defconfig

先看 `build.sh`：

```bash
grep -n 'defconfig' build.sh
```

假设输出类似：

```text
make xxx_defconfig
```

检查这个文件：

```bash
ls -l configs/xxx_defconfig
```

保存：

```bash
cp configs/xxx_defconfig \
  ~/work/linux/bsp/imx6ull/artifacts/uboot/
```

然后比较最终 `.config`：

```bash
diff -u \
  configs/xxx_defconfig \
  .config \
  | head -120 || true
```

不要求两者完全一样；`.config` 会经过 Kconfig 展开。这里是为了知道：
- 输入配置是什么；
- 最终展开结果是什么。

---

## 9.15 查网络/TFTP 命令是否编进 U-Boot

最终 `.config`：

```bash
grep -E \
  'CONFIG_CMD_PING|CONFIG_CMD_TFTP|CONFIG_NET|CONFIG_CMD_DHCP' \
  .config
```

不同旧版 U-Boot symbol 名可能不同。

如果 grep 无结果，再：

```bash
grep -R \
  'CMD_TFTP\|CMD_PING' \
  include configs .config \
  | head -80
```

目的是确认 Day 4 依赖的 `ping` / `tftp` 命令不是碰运气。

---

## 9.16 对比“正在板上运行的 U-Boot”和“刚编出来的 U-Boot”

Target 串口：

```bash
version
```

Host：

```bash
strings \
  ~/work/linux/bsp/imx6ull/artifacts/uboot/u-boot.imx \
  | grep -m3 -i 'U-Boot'
```

如果输出能看到版本字符串，记录：

```text
running U-Boot:
built U-Boot:
```

即使本周不烧写，也要知道二者是否来自同一资料版本。

---

## 9.17 检查 image 大小变化

```bash
cd ~/work/linux/bsp/imx6ull/artifacts/uboot

stat -c '%n %s bytes' u-boot.imx
```

保存：

```bash
stat -c '%n %s bytes' u-boot.imx \
  | tee image_size.txt
```

以后修改 U-Boot 配置导致镜像突然增大时，这个 baseline 很有价值。

---

## 9.18 验证 build 可重复

不要修改源码，再跑一次：

```bash
cd "$UBOOT_SRC"

./build.sh 2>&1 | tee \
  ~/work/linux/bsp/imx6ull/build_logs/uboot_rebuild.log
```

重新复制：

```bash
cp tmp/u-boot.imx \
  ~/work/linux/bsp/imx6ull/artifacts/uboot/u-boot.imx.rebuild
```

计算：

```bash
cd ~/work/linux/bsp/imx6ull/artifacts/uboot

sha256sum \
  u-boot.imx \
  u-boot.imx.rebuild
```

如果 hash 不同，不要立即判定“构建不稳定”；老 U-Boot 可能嵌入时间戳。先用：

```bash
cmp -l u-boot.imx u-boot.imx.rebuild | head
```

记录差异即可。

本周要求的是“流程可重复”，不是要求 binary 必须 bit-for-bit identical。

## 9.14 Day 2 验收

```bash
test -f   ~/work/linux/bsp/imx6ull/artifacts/uboot/u-boot.imx
echo $?
```

必须：

```text
0
```

然后：

```bash
sha256sum   ~/work/linux/bsp/imx6ull/artifacts/uboot/u-boot.imx
```

---

## 9.15 资料

- 正点原子快速体验 **4.3 编译出厂源码 U-Boot**
- 官方流程：源码 -> Poky -> `build.sh` -> `tmp/u-boot.imx`
- [4.3 Online](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/u-boot/)
