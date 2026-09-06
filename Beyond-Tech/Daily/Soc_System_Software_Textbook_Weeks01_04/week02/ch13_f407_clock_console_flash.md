# Chapter 13 - Bring the STM32F407 Board Alive: Clock, USART1 Console and Flashing

> Week 2 / Day 6 - 让自定义 board 从“能配置”变成“真实硬件可输出、可烧录”。

[← Part README](README.md) · [← Previous](ch12_zephyr_out_of_tree_board.md) · [Next →](ch14_week2_integration.md)

## 13.1 Bring-up 的正确顺序：Clock -> Pinmux -> UART -> Console -> Flash/Debug

一块新板第一次上 Zephyr，不要从 LED 开始。LED 不亮可能有十几个原因，而串口 console 一旦工作，就能让后续错误变得可观察。

```text
Reset
 -> HSE/PLL/system clock
 -> pinctrl selects USART1 pins
 -> UART driver init
 -> console subsystem binds chosen UART
 -> printk/log output
```

## 13.2 Clock：8 MHz HSE 是板级事实，不是“STM32F407 默认值”

原理图 p.2：Y2 = 8 MHz。你的 DTS/clock-control 配置必须与该事实一致。

费曼模型：PLL 像齿轮箱，配置倍频/分频之前必须知道输入轴转速。如果 HSE 实际 8 MHz，却把 board 描述成 25 MHz，即使编译通过，系统时钟、UART baud、timer 都可能错。

今天不要背 STM32 RCC 所有寄存器；学会从：

```text
schematic oscillator -> DTS clock node -> clock_control driver -> final SYSCLK
```

验证。

## 13.3 Pinctrl：UART controller 存在不等于 PA9/PA10 已经接给它

STM32 pin 是复用资源。p.2：PA9/PA10 有 USART1 alternate function。board DTS 需要选择正确 pinctrl state。

检查最终输出：

```bash
grep -n 'usart1\|pinctrl' build/zephyr/zephyr.dts | head -60
```

不要只看源 DTS；最终 `zephyr.dts` 才能证明 include/overlay 合并后的真实结果。

## 13.4 `chosen`：为什么 console 不应该由应用硬编码 device name

```dts
chosen {
    zephyr,console = &usart1;
};
```

`chosen` 表达系统角色：“谁承担 console”。应用和 subsystems 读取这个角色，而不是到处知道具体 UART1。

这与 Linux `chosen { stdout-path = ...; }` 的思想相似，但 Zephyr 消费方式主要发生在 build-time。

## 13.5 Worked Example：让 Hello World 从板载 CH340 路径输出

上电前确认：

- P6/跳帽连接符合板子原理图；
- CH340 USB 正确枚举；
- Host 找到 `/dev/ttyUSBx`；
- DTS `usart1` status okay；
- console/serial 对应 Kconfig 开启。

构建：

```bash
west build -p always -b f407_explorer app/hello -- -DBOARD_ROOT=<your-platform-root>
```

烧录方式取决于你实际 J-Link/ST-Link/OpenOCD probe。第一目标是**可重复烧录**，不是限定某品牌探针。

## 13.6 烧录与调试 runner：理解命令背后是谁执行

```bash
west flash
west debug
```

west 不是自己实现 SWD/JTAG protocol，它选择 board 配置的 runner，再调用 OpenOCD/J-Link 等后端。

如果 `west flash` 报 “runner not available”，这是 board/tool integration 层；如果 probe 连不上芯片，是 debug transport/hardware 层。不要混查。

## 13.7 Guided Lab：四层证明 console 路径

依次验证：

1. **Hardware**：CH340 枚举，跳帽/网络正确；
2. **DTS**：最终 `zephyr.dts` 的 usart1/pinctrl/chosen 正确；
3. **Kconfig**：`.config` 有 serial/console；
4. **Runtime**：串口输出 banner/hello。

每层留下一个证据文件/截图/命令输出。

## 13.8 故障注入：将 console baud 或 pinctrl 改错

一次只改一个。观察：

- 无输出；
- 乱码；
- build-time error；

分别对应哪些层？恢复后 clean build 再验。

## 13.9 Independent Challenge：不用 sample，写 20 行以内最小 app

只做：

```c
printk("F407 Explorer alive\n");
```

同时打印 build/version string。目标是证明你的 board port 不依赖某个 sample 的特殊配置。

## 13.10 下一章：现在 Linux 与 Zephyr 都能从 clean source 生成并上板，必须把两套“硬件描述”放到同一张图上

Chapter 14 做本周集成：删除 build 重新构建 6ULL DTB 与 Zephyr app；比较 Linux DTS runtime population 与 Zephyr DTS build-time generation，为后续 DeviceTree 深入打地基。

## References and manuals

### STM32F407 Explorer V2.2 Schematic
- Local expected path: `../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf`
- 本章阅读定位：p.2：8MHz HSE、USART1 PA9/PA10；p.4：CH340G；结合实板跳帽验证。

### Zephyr Board Porting Guide
- Online: [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- 本章阅读定位：看 flashing/debug runner 与 board.cmake/metadata 当前版本要求。

### Zephyr Devicetree
- Online: [Zephyr Devicetree](https://docs.zephyrproject.org/latest/build/dts/index.html)
- 本章阅读定位：重点看 chosen、pinctrl、最终 zephyr.dts。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch12_zephyr_out_of_tree_board.md) · [Next →](ch14_week2_integration.md)
