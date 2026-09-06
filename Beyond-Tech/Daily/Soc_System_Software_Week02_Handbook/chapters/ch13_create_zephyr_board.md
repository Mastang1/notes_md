# Chapter 13 - 创建 Explorer STM32F407 out-of-tree Board

## 13.1 复制 starter project

```bash
mkdir -p ~/work/zephyr/apps

cp -a <week2-package>/zephyr_app   ~/work/zephyr/apps/f407_explorer_smoke

cd ~/work/zephyr/apps/f407_explorer_smoke

tree
```

预期：

```text
.
├── CMakeLists.txt
├── prj.conf
├── src/
└── boards/
    └── others/
        └── alientek_f407_explorer/
```

---

## 13.2 激活 Zephyr v4.4.1

```bash
source ~/work/zephyr/activate.sh
```

确认：

```bash
cd ~/work/zephyr/workspace/zephyr
git describe --tags --always
```

应是课程固定 v4.4.1 基线。

---

## 13.3 打开 `board.yml`

回 app：

```bash
cd ~/work/zephyr/apps/f407_explorer_smoke

cat   boards/others/alientek_f407_explorer/board.yml
```

内容：

```yaml
board:
  name: alientek_f407_explorer
  full_name: ALIENTEK Explorer STM32F407
  vendor: others
  socs:
    - name: stm32f407xx
```

Zephyr v4.4.1 官方 `black_f407ve` 的 `board.yml` 使用同一个：

```text
stm32f407xx
```

---

## 13.4 检查 Kconfig SoC

```bash
cat   boards/others/alientek_f407_explorer/Kconfig.alientek_f407_explorer
```

应：

```text
config BOARD_ALIENTEK_F407_EXPLORER
    select SOC_STM32F407XE
```

你的芯片是：

```text
STM32F407ZET6
```

这里 `XE` 对应 512 KB Flash variant support。

---

## 13.5 检查 SoC DTSI 存在

```bash
test -f   ~/work/zephyr/workspace/zephyr/dts/arm/st/f4/stm32f407Xe.dtsi

echo $?
```

必须：

```text
0
```

---

## 13.6 找 Z package pinctrl include

```bash
find ~/work/zephyr/workspace/zephyr/dts   -name '*stm32f407z*pinctrl.dtsi'   -print
```

starter 使用：

```text
stm32f407z(e-g)tx-pinctrl.dtsi
```

如果你 v4.4.1 checkout 的真实文件名有差异：
- 以 `find` 结果为准；
- 修改 starter DTS include；
- 不要复制 V-package 的 pinctrl include。

---

## 13.7 检查 USART1 pinctrl symbol

```bash
grep -R   'usart1_tx_pa9'   ~/work/zephyr/workspace/zephyr/dts   | head -20
```

再：

```bash
grep -R   'usart1_rx_pa10'   ~/work/zephyr/workspace/zephyr/dts   | head -20
```

必须能找到定义。

---

## 13.8 检查 Explorer DTS

```bash
nl -ba   boards/others/alientek_f407_explorer/alientek_f407_explorer.dts
```

逐项核：

```text
stm32f407Xe.dtsi
HSE 8 MHz
PLL 168 MHz
USART1 PA9/PA10
115200
LED0 PF9 active-low
LED1 PF10 active-low
```

任何一项有疑问，回：
- Week 1 Board Audit；
- Explorer 原理图 p.2/p.3/p.4。

---

## 13.9 `BOARD_ROOT` 指向哪里

当前 app：

```bash
pwd
```

假设：

```text
~/work/zephyr/apps/f407_explorer_smoke
```

这个目录下面直接有：

```text
boards/
```

所以：

```bash
BOARD_ROOT="$PWD"
```

不是：

```text
.../boards/others/alientek_f407_explorer
```

---

## 13.10 让 west 列出 custom board

```bash
west boards --board-root "$PWD" |   grep alientek_f407_explorer
```

如果当前 `west boards` 参数不同，不在这里卡死，直接做 configure/build 验证。

---

## 13.11 第一次 clean build

```bash
west build -p always   -b alientek_f407_explorer   .   -- -DBOARD_ROOT="$PWD"
```

保存：

```bash
mkdir -p ~/work/zephyr/logs

west build -p always   -b alientek_f407_explorer   .   -- -DBOARD_ROOT="$PWD"   2>&1 | tee   ~/work/zephyr/logs/week2_custom_board_build.log
```

---

## 13.12 检查 `.config`

```bash
grep -E   '^CONFIG_BOARD|^CONFIG_SOC|^CONFIG_SERIAL|^CONFIG_GPIO'   build/zephyr/.config
```

重点看到：
- custom board；
- STM32F407XE SoC；
- serial/gpio。

---

## 13.13 检查最终 DTS

```bash
grep -nE   'model =|compatible =|current-speed|gpiof|serial@'   build/zephyr/zephyr.dts | head -100
```

检查 chosen：

```bash
grep -n -A15   'chosen'   build/zephyr/zephyr.dts | head -40
```

必须 console -> USART1。

---

## 13.14 检查 ELF

```bash
file build/zephyr/zephyr.elf
```

再：

```bash
arm-zephyr-eabi-readelf   -h build/zephyr/zephyr.elf   | grep -E 'Class:|Machine:|Entry'
```

---

## 13.15 Board 不被发现

```bash
pwd
find boards -maxdepth 4 -type f -print
```

再：

```bash
west build -p always   -b alientek_f407_explorer   .   -- -DBOARD_ROOT="$(pwd)"
```

---

## 13.16 SoC Kconfig 失败

```bash
grep -R   'SOC_STM32F407XE'   ~/work/zephyr/workspace/zephyr   | head -30
```

对照官方：

```text
black_f407ve
```

---

## 13.17 DTS include 不存在

```bash
find ~/work/zephyr/workspace/zephyr/dts   -iname '*407*' | sort | head -100
```

严格用当前 v4.4.1 checkout 的真实文件。

---

## 13.18 Day 6 验收

```text
[ ] board.yml 被识别
[ ] SOC_STM32F407XE
[ ] stm32f407Xe.dtsi
[ ] Z-package pinctrl include
[ ] USART1 PA9/PA10
[ ] HSE 8 MHz
[ ] PF9/PF10
[ ] clean build
[ ] .config 正确
[ ] zephyr.dts 正确
[ ] zephyr.elf 生成
```

---

## 13.19 资料

- [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- [Custom Board / BOARD_ROOT](https://docs.zephyrproject.org/latest/develop/application/index.html#custom-board-devicetree-and-soc-definitions)
- [v4.4.1 board.yml reference](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/board.yml)
- [v4.4.1 Kconfig reference](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/Kconfig.black_f407ve)
- [v4.4.1 DTS reference](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.4.1/boards/others/black_f407ve/black_f407ve.dts)
