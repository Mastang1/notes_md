# Chapter 14 - Explorer F407 Bring-up：Clock -> USART1 console -> LED -> Week 2 Gate

## 14.1 今天只 bring-up 三样

固定顺序：

```text
1. SWD/JTAG 能连
2. Clock 正常
3. USART1 console 正常
4. LED0/LED1 正常
```

今天不碰：
- W25Q128；
- KEY；
- Ethernet；
- MCUboot。

---

## 14.2 先接 USB-UART

Explorer 原理图：
- p.2：MCU `USART1_TX/RX`，PA9/PA10；
- p.4：CH340G；
- p.2/p.4：中间经过 P6。

Host：

```bash
sudo dmesg -w
```

插 USB-UART。

另一个 shell：

```bash
ls -l /dev/ttyUSB*
```

如果多个：

```bash
for d in /dev/ttyUSB*; do
  echo "=== $d ==="
  udevadm info --query=property --name="$d" |     grep -E 'ID_VENDOR_ID|ID_MODEL_ID|ID_SERIAL'
done
```

---

## 14.3 接 SWD/JTAG

### J-Link

连接：
- SWDIO -> TMS/SWDIO
- SWCLK -> TCK/SWCLK
- GND -> GND
- VTref -> 3.3V reference
- NRST 可选

不要让调试器和板载电源产生冲突。

先：

```bash
JLinkExe
```

交互：

```text
device STM32F407ZE
if SWD
speed 4000
connect
```

能读 core/register 就继续。

---

## 14.4 ST-Link/OpenOCD 路径

如果用 ST-Link：

```bash
openocd   -f interface/stlink.cfg   -f target/stm32f4x.cfg
```

另一个 shell：

```bash
nc localhost 4444
```

输入：

```text
reset halt
reg
exit
```

若找不到 cfg：

```bash
find /usr/share/openocd   \( -name stlink.cfg -o -name stm32f4x.cfg \)   -print
```

---

## 14.5 Clean build

```bash
source ~/work/zephyr/activate.sh

cd ~/work/zephyr/apps/f407_explorer_smoke

west build -p always   -b alientek_f407_explorer   .   -- -DBOARD_ROOT="$PWD"
```

检查：

```bash
file build/zephyr/zephyr.elf
ls -lh   build/zephyr/zephyr.elf   build/zephyr/zephyr.bin   build/zephyr/zephyr.hex
```

---

## 14.6 检查 Clock DTS

```bash
grep -n -A20 '&clk_hse'   boards/others/alientek_f407_explorer/alientek_f407_explorer.dts
```

再：

```bash
grep -n -A25 '&pll'   boards/others/alientek_f407_explorer/alientek_f407_explorer.dts
```

再：

```bash
grep -n -A20 '&rcc'   boards/others/alientek_f407_explorer/alientek_f407_explorer.dts
```

本周目标值：

```text
HSE = 8 MHz
PLL M = 8
PLL N = 336
PLL P = 2
SYSCLK = 168 MHz
APB1 = /4
APB2 = /2
```

---

## 14.7 检查 USART1 DTS

```bash
grep -n -A12 '&usart1'   boards/others/alientek_f407_explorer/alientek_f407_explorer.dts
```

必须：

```text
TX = PA9
RX = PA10
current-speed = 115200
status = okay
```

最终 DTS 再查：

```bash
grep -n -A12 'chosen'   build/zephyr/zephyr.dts | head -40
```

---

## 14.8 J-Link flash

starter `board.cmake` 提供 J-Link runner。

先：

```bash
west flash -H
```

再：

```bash
west flash -r jlink
```

如果报 J-Link device：

```bash
west flash -H -r jlink
```

检查 runner 参数。

---

## 14.9 ST-Link/OpenOCD 手工 flash

如果不用 J-Link：

```bash
openocd   -f interface/stlink.cfg   -f target/stm32f4x.cfg   -c "program build/zephyr/zephyr.elf verify reset exit"
```

如果 OpenOCD scripts 不在默认目录：

```bash
openocd --version
find /usr/share/openocd   -name stm32f4x.cfg -o -name stlink.cfg
```

再加：

```bash
-s /usr/share/openocd/scripts
```

---

## 14.10 打开 USART1 console

先确认实际 CH340 设备：

```bash
ls -l /dev/ttyUSB*
```

打开：

```bash
picocom -b 115200 --flow n /dev/ttyUSB0
```

复位板子。

预期：

```text
week2 f407 explorer smoke
led0 ready=1
led1 ready=1
count=0
count=1
...
```

---

## 14.11 Console 完全没字：按这个顺序查

### Step 1：程序是否写进去

J-Link/OpenOCD 看 program/verify log。

### Step 2：MCU 是否运行

J-Link：

```text
halt
regs
go
```

或 GDB 看 PC 是否变化。

### Step 3：P6

按 Explorer 原理图检查 USART1 到 CH340 的 P6 路径是否接通。

### Step 4：PA9 波形

示波器/逻辑分析仪接：
- GND
- PA9 / USART1_TX 可测点

持续 `printk()` 时应该有跳变。

### Step 5：115200 bit time

115200 baud：

```text
1 bit ~= 8.68 us
```

如果 PA9 有正确 UART：
- firmware/UART/clock 大概率正常；
- 查 P6/CH340/USB/ttyUSB。

如果 PA9 没波形：
- 查 chosen；
- usart1 status；
- pinctrl；
- clock；
- 程序是否跑到 main。

---

## 14.12 LED 验证

程序会 toggle：
- LED0 PF9 active-low
- LED1 PF10 active-low

检查 final DTS：

```bash
grep -n -A30 'compatible = "gpio-leds"'   build/zephyr/zephyr.dts
```

应该看到 PF9/PF10 和 active-low flag。

---

## 14.13 LED 不亮但 Console 正常

先看程序 log：

```text
led0 ready=1
led1 ready=1
```

如果 ready=0：
- GPIO controller/DTS 问题。

如果 ready=1：
- 回 final DTS；
- 回原理图 p.2/p.3；
- 测 PF9/PF10 实际电平。

---

## 14.14 用示波器验证 LED GPIO

程序 500 ms toggle。

测 PF9：

```text
应看到约 1 Hz 周期电平翻转
```

测 PF10 同理。

如果 GPIO 在翻但 LED 不亮：
- 硬件路径/LED/电阻/极性；
- 不是 Zephyr scheduler 问题。

---

## 14.15 J-Link `west debug`

```bash
west debug -r jlink
```

GDB：

```gdb
break main
continue
next
info registers
```

退出：

```gdb
quit
```

---

## 14.16 OpenOCD + GDB

OpenOCD：

```bash
openocd   -f interface/stlink.cfg   -f target/stm32f4x.cfg
```

另一个 shell：

```bash
arm-zephyr-eabi-gdb   build/zephyr/zephyr.elf
```

GDB：

```gdb
target extended-remote localhost:3333
monitor reset halt
load
break main
continue
```

---

## 14.17 保存 bring-up 证据

目录：

```bash
mkdir -p   ~/work/zephyr/logs/week2_f407
```

保存：
- build log；
- flash log；
- serial log；
- final `zephyr.dts`；
- final `.config`。

例如：

```bash
cp build/zephyr/zephyr.dts   ~/work/zephyr/logs/week2_f407/

cp build/zephyr/.config   ~/work/zephyr/logs/week2_f407/
```

串口：

```bash
script -f   ~/work/zephyr/logs/week2_f407/serial.log   -c "picocom -b 115200 --flow n /dev/ttyUSB0"
```

---

## 14.18 Linux DTB 和 Zephyr DTS 做一次操作对比

Linux：

```bash
dtc -I dtb -O dts   ~/work/linux/bsp/imx6ull/artifacts/kernel/imx6ull-alientek-emmc.dtb   -o /tmp/imx6ull-week2.dts
```

Zephyr：

```bash
cp build/zephyr/zephyr.dts   /tmp/f407-week2.dts
```

只 grep：

```bash
grep -nE   'chosen|compatible|serial|gpio|clock'   /tmp/imx6ull-week2.dts | head -100
```

再：

```bash
grep -nE   'chosen|compatible|serial|gpio|clock'   /tmp/f407-week2.dts | head -100
```

理论先不展开，记录看到的差异即可。

---

## 14.19 Week 2 Gate - Linux

```text
[ ] U-Boot source/version 记录
[ ] U-Boot build.sh 成功
[ ] u-boot.imx + SHA256
[ ] Kernel source/version 记录
[ ] zImage
[ ] MINI DTB
[ ] modules 输出记录
[ ] TFTP zImage
[ ] TFTP DTB
[ ] bootz 成功
[ ] uname 证明新 Kernel
```

---

## 14.20 Week 2 Gate - Debug

```text
[ ] 有 gdbserver：remote breakpoint 完成
或
[ ] 无 gdbserver：记录 factory rootfs + vendor SDK 搜索结果
```

---

## 14.21 Week 2 Gate - Zephyr

```text
[ ] alientek_f407_explorer 被发现
[ ] SOC_STM32F407XE
[ ] HSE 8 MHz
[ ] USART1 PA9/PA10
[ ] console 115200
[ ] LED0 PF9 active-low
[ ] LED1 PF10 active-low
[ ] clean build
[ ] flash
[ ] serial log
[ ] LED toggle
```

---

## 14.22 Git 收口

建议目录：

```text
week02/
├── bsp/
├── logs/
├── gdb/
├── zephyr_app/
└── README.md
```

Commit：

```text
feat: complete week2 bsp build and f407 board bringup
```

---

## 14.23 本章验收

本章必须有实际证据：

```text
[ ] Debug probe 能连接 STM32F407ZE
[ ] Custom board clean build 成功
[ ] Flash verify 成功
[ ] USART1 console 有连续输出
[ ] PA9 UART 波形可测
[ ] LED0/LED1 实际翻转
[ ] final zephyr.dts 已保存
[ ] final .config 已保存
[ ] serial.log 已保存
```

如果只有 build 成功、没有真实板输出，本章按未完成处理。

## 14.24 资料

- [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- [west flash/debug](https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html)
- [Devicetree HOWTO](https://docs.zephyrproject.org/latest/build/dts/howtos.html)
- [Explorer schematic](../references/Explorer_STM32F4_V2.2_SCH.pdf)
