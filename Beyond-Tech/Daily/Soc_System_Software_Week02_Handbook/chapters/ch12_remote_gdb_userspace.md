# Chapter 12 - `gdbserver + gdb-multiarch`：远程单步 ARM 用户程序

## 12.1 今天先检查 Target 是否有 `gdbserver`

Target：

```bash
which gdbserver
gdbserver --version
```

### 有

继续。

### 没有

执行：

```bash
find / -name gdbserver -type f 2>/dev/null | head -30
```

若仍没有，记录：

```text
factory rootfs has no gdbserver
```

不要从 Ubuntu Host 复制：

```text
/usr/bin/gdbserver
```

因为它通常是 x86_64。

优先：
1. 查正点原子 Poky SDK/sysroot；
2. 查配套 rootfs 的 target packages；
3. 找与当前 ARM rootfs ABI 匹配的 gdbserver。

如果暂时没有，本章做到“Host debug ELF + Target gdbserver dependency 记录”，不要用错误 binary 假完成。

---

## 12.2 复制本包 demo

Host：

```bash
mkdir -p ~/work/linux/debug/week2_gdb

cp -a <week2-package>/labs/gdb_demo/*   ~/work/linux/debug/week2_gdb/

cd ~/work/linux/debug/week2_gdb
ls -la
```

---

## 12.3 编译 debug 版本

默认：

```bash
make
```

或者直接：

```bash
arm-linux-gnueabihf-gcc   -O0 -g3 -Wall -Wextra   main.c -o hello_dbg
```

如果 Target 出厂 rootfs 需要 Poky ABI，改：

```bash
make CROSS_COMPILE=arm-poky-linux-gnueabi-
```

---

## 12.4 检查 ELF

```bash
file hello_dbg
readelf -h hello_dbg |   grep -E 'Class:|Machine:|Entry'
```

确认 debug section：

```bash
readelf -S hello_dbg |   grep -E 'debug_info|debug_line|debug_abbrev'
```

必须看到 DWARF 相关 section。

---

## 12.5 先正常运行

复制到 NFS：

```bash
cp hello_dbg /srv/nfs/imx6ull/
sync
```

Target：

```bash
cd /mnt/nfs
chmod +x hello_dbg
./hello_dbg
```

先正常运行成功。

如果这里出现：

```text
No such file or directory
```

但文件存在，检查：

```bash
readelf -l hello_dbg | grep -A1 INTERP
ls -l /lib/ld-linux* 2>/dev/null
```

先解决 loader/ABI。

---

## 12.6 Target 启动 `gdbserver`

Target：

```bash
cd /mnt/nfs

gdbserver :2345 ./hello_dbg
```

应：

```text
Listening on port 2345
```

此时程序等待 Host。

---

## 12.7 Host 测 TCP

```bash
nc -vz <target-ip> 2345
```

失败时 Target 查：

```bash
ss -lnt 2>/dev/null | grep 2345
netstat -lnt 2>/dev/null | grep 2345
```

---

## 12.8 Host 启动 GDB

```bash
cd ~/work/linux/debug/week2_gdb
gdb-multiarch ./hello_dbg
```

GDB：

```gdb
set architecture arm
target remote <target-ip>:2345
```

---

## 12.9 断点 `main`

```gdb
break main
continue
```

再：

```gdb
list
backtrace
info registers
```

---

## 12.10 单步

```gdb
next
next
print counter
print result
```

进入 `calc()`：

```gdb
step
```

退出：

```gdb
finish
```

---

## 12.11 改变量

```gdb
print counter
set variable counter = 100
print counter
next
```

继续观察：

```gdb
print result
```

---

## 12.12 看局部变量

```gdb
info locals
```

看函数参数：

```gdb
info args
```

看源代码：

```gdb
list
```

---

## 12.13 看变量地址

```gdb
p &counter
x/4wx &counter
```

如果支持：

```gdb
info proc mappings
```

不支持就记录，不需要折腾。

---

## 12.14 保存 GDB session

退出 GDB，改用：

```bash
script -f   ~/work/linux/bsp/imx6ull/build_logs/week2_gdb_session.log   -c "gdb-multiarch ./hello_dbg"
```

重新做一遍：

```gdb
set architecture arm
target remote <target-ip>:2345
break main
continue
next
print counter
backtrace
```

---

## 12.15 常见失败：`Remote connection closed`

Target 看：

```bash
ps | grep gdbserver
```

重启：

```bash
gdbserver :2345 ./hello_dbg
```

GDB remote session 默认是一轮进程调试；程序退出后 server 可能退出。

---

## 12.16 常见失败：source 不匹配

Host：

```bash
sha256sum hello_dbg
```

Target：

```bash
sha256sum /mnt/nfs/hello_dbg 2>/dev/null || true
```

必须是同一个 build。

---

## 12.17 常见失败：`optimized out`

检查 Makefile：

```bash
grep CFLAGS Makefile
```

必须：

```text
-O0 -g3
```

重新：

```bash
make clean
make
```

---

## 12.18 如果动态库符号不对，配置 sysroot

Host 的 GDB 调 Target 动态程序时，最好使用与 Target rootfs 对应的 sysroot。

如果 NFS 可访问 rootfs 的复制目录，例如：

```text
~/work/linux/bsp/imx6ull/rootfs/
```

GDB 中：

```gdb
set sysroot /home/<user>/work/linux/bsp/imx6ull/rootfs
```

查看：

```gdb
show sysroot
```

如果没有完整 rootfs copy，本周不强制配置。

---

## 12.19 检查 `sharedlibrary`

GDB remote connect 后：

```gdb
info sharedlibrary
```

如果显示：
- library 已加载但 symbols missing；
- 路径找不到；

再考虑 sysroot/solib-search-path。

例如：

```gdb
set solib-search-path \
/home/<user>/work/linux/bsp/imx6ull/rootfs/lib:\
/home/<user>/work/linux/bsp/imx6ull/rootfs/usr/lib
```

然后：

```gdb
sharedlibrary
info sharedlibrary
```

---

## 12.20 如果 `gdbserver` 不在 PATH，先找 SDK

Host：

```bash
find ~/work \
  -type f -name gdbserver \
  2>/dev/null | head -50
```

如果安装过 Poky SDK：

```bash
find /opt \
  -type f -name gdbserver \
  2>/dev/null | head -50
```

对候选先：

```bash
file <candidate-gdbserver>
```

必须看到 ARM target，不是 x86-64。

---

## 12.21 拷贝候选 gdbserver 前先查动态依赖

Host：

```bash
readelf -l <candidate-gdbserver> | \
  grep -A1 INTERP
```

如果是动态 ELF，再：

```bash
readelf -d <candidate-gdbserver> | \
  grep NEEDED
```

如果依赖与 Target rootfs 不匹配，不要复制。

---

## 12.22 网络调试只开放在开发 LAN

`gdbserver :2345` 没有做复杂认证。

本周只在可信开发网里使用。

结束后确认：

```bash
ps | grep gdbserver
```

如果残留：

```bash
kill <pid>
```

Host：

```bash
nc -vz <target-ip> 2345
```

应不再连接成功。

---

## 12.23 保存本次调试命令

建立：

```bash
cat > ~/work/linux/debug/week2_gdb/gdb_commands.txt <<'EOF'
set architecture arm
target remote <target-ip>:2345
break main
continue
next
print counter
step
backtrace
EOF
```

下次不要重新回忆命令。

---

## 12.24 复现一次“二进制和符号不一致”

Host 先备份：

```bash
cp hello_dbg hello_dbg.good
```

修改源码后重新 build：

```bash
make clean
make
```

但 Target 仍运行旧 `hello_dbg.good`。

Host 用新 ELF 去连旧进程时，源码/地址可能对不上。

实验结束后恢复同一 build：

```bash
cp hello_dbg /srv/nfs/imx6ull/
```

这一步让你实际看到：

> Remote GDB 的 Host ELF 必须和 Target 正在执行的 binary 对应。

## 12.18 Day 5 验收

如果有 gdbserver：

```text
[ ] target remote
[ ] break main
[ ] continue
[ ] next
[ ] step calc
[ ] print counter
[ ] set variable counter
[ ] backtrace
[ ] 保存 session log
```

如果没有：
- 记录 `which gdbserver`；
- 记录 factory rootfs；
- 在 vendor SDK 中实际搜索一次：

```bash
find <poky-sdk-root> -name gdbserver -type f 2>/dev/null
```

---

## 12.19 本章产物

```text
~/work/linux/debug/week2_gdb/
├── main.c
├── Makefile
└── hello_dbg

~/work/linux/bsp/imx6ull/build_logs/
└── week2_gdb_session.log
```
