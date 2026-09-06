# Chapter 19 - Describe STM32F407 Devices in Zephyr: LED, KEY and Devicetree

> Week 3 / Day 5 - 用真实原理图把板级设备转成 Zephyr 角色对象。

[← Part README](README.md) · [← Previous](ch18_file_descriptor_uapi.md) · [Next →](ch20_zephyr_west_debug.md)

## 19.1 从 Week 1 Board Audit 取回两个硬件事实

本章只增加两个功能：LED、KEY。原理图已验证：

- LED0 -> PF9，LED1 -> PF10，典型 active-low；
- KEY0 -> PE4，KEY1 -> PE3，KEY2 -> PE2，WK_UP -> PA0。

所有代码设计从这几行事实开始，而不是 sample 开始。

## 19.2 `gpio-leds`：为什么一个 LED 不需要你写自定义 driver

Zephyr 已有 binding/通用 API。板级 DTS 只需要描述一个 GPIO consumer：

```dts
/ {
    leds {
        compatible = "gpio-leds";
        led0: led_0 {
            gpios = <&gpiof 9 GPIO_ACTIVE_LOW>;
            label = "LED0";
        };
    };

    aliases {
        led0 = &led0;
    };
};
```

具体 controller label/pinctrl 以 Zephyr STM32 DTSI 为准；以上用于解释结构，不要不查最终 DTS 就复制。

费曼模型：`gpio-leds` 告诉 build system“这是一个由 GPIO 控制的 LED 设备类型”，`gpios` 告诉它实际接线与极性，alias 给应用一个稳定角色名。

## 19.3 Active Low 为什么应该进入 DTS

如果应用写：

```c
gpio_pin_set(..., !on);
```

说明业务逻辑知道了 PCB 电气极性。换板后每个调用点都要改。

让 DTS 用 `GPIO_ACTIVE_LOW` 表达后，logical API 可以统一 `1=active`。**硬件差异留在 board description。**

## 19.4 Binding：它不是“另一个 DTS 文件”

Binding（YAML）定义 compatible 对应节点应该/可以有哪些 property，specifier cells 如何解释。Build system 用 binding 校验并生成 typed metadata。

查看：

```bash
west build ...
grep -n 'led_0\|gpio-leds' build/zephyr/zephyr.dts
```

再查 generated devicetree headers 中对应宏。不要手改 generated files。

## 19.5 Worked Example：用 `DT_ALIAS(led0)` 获取板级角色

典型：

```c
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
```

接着：

```c
if (!gpio_is_ready_dt(&led)) { ... }
gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
gpio_pin_toggle_dt(&led);
```

这里真正重要的是：应用没有硬编码 `GPIOF,9`。

## 19.6 KEY：输入极性和 pull/debounce 是不同层的问题

DTS 表达 pin/active level；pull-up/down 可以是 pinctrl/GPIO flags/板级电气的一部分，必须核对实际电路。**Debounce 则是信号处理/driver/application policy，不应该因为按键 active-low 就混成一个概念。**

第一版先轮询读取：

```c
gpio_pin_get_dt(&key)
```

确认按下/释放 logical value。IRQ 留到后续 Linux/Zephyr IRQ 专题。

## 19.7 Guided Lab：blinky + button 两个实验合并成一个 board smoke app

应用结构：

```text
startup
 -> verify LED GPIO ready
 -> verify KEY GPIO ready
 -> LED heartbeat
 -> key pressed: print event + toggle LED1
```

通过串口打印 logical value。用万用表/示波器可选观察 PF9 电平，验证 ACTIVE_LOW 语义。

## 19.8 故障实验：把 LED 极性故意写反

程序仍会运行，但 logical “ON” 与物理表现相反。这个错误不会被 compiler/DT binding 发现，因为连接在语法上完全合法。

结论：**binding 能验证数据结构，不替代 schematic/hardware truth。**

## 19.9 Independent Challenge：从最终 `zephyr.dts` 反查回原理图

选择 LED0：

```text
zephyr.dts node
 -> controller &gpiof
 -> pin 9
 -> GPIO_ACTIVE_LOW
 -> schematic p.2/p.3 PF9 / LED0 net
```

写成一条证据链。下次换板就按同样路径，不靠记忆。

## 19.10 下一章：板级对象已经正常，现在学习如何在真实 Cortex-M 上停下来观察 thread/register/stack

Chapter 20 用 west debug/GDB。重点是把你会用的 JTAG/Trace32 调试经验迁移到 Zephyr runner/GDB，而不是重新学断点。

## References and manuals

### STM32F407 Explorer V2.2 Schematic
- Local expected path: `../references/ALIENTEK_Explorer_STM32F4_V2.2_Schematic.pdf`
- 本章阅读定位：p.2/p.3：PF9/PF10 LED、PE2/3/4 keys；所有 pin/polarity 以原理图与实测为准。

### Zephyr Devicetree
- Online: [Zephyr Devicetree](https://docs.zephyrproject.org/latest/build/dts/index.html)
- 本章阅读定位：重点 node/alias/binding/generated DT。

### Zephyr GPIO API
- Online: [Zephyr GPIO API](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- 本章阅读定位：重点 gpio_dt_spec/DT helper 的板级解耦思想。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch18_file_descriptor_uapi.md) · [Next →](ch20_zephyr_west_debug.md)
