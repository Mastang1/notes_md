# W03D05 - Zephyr LED/KEY Devicetree: aliases, gpio-leds, gpio-keys

## 0. 今日定位

- 主线：Zephyr board/device model
- 时间：2h
- 硬件：Explorer STM32F407ZET6
- 前置：Week2 custom board console works
- 产物：board DTS commit + LED/KEY smoke test

## 1. 今天解决的工程问题

今天不是学 GPIO 时序，而是把真实原理图资源正确表达成 Zephyr Devicetree，并让通用 GPIO API 消费它。

## 2. 今日能力构成

```mermaid
flowchart LR
    SCH[Schematic] --> DTS[board DTS]
    DTS --> BIND[gpio-leds/gpio-keys binding]
    BIND --> GEN[generated DT data]
    GEN --> SPEC[gpio_dt_spec]
    SPEC --> GPIO[generic GPIO driver]
```

## 3. 先理解：费曼解释

### 3.1 30 秒白话模型

DTS 就像把“PF9 接 LED0，低电平亮”写成机器可消费的硬件说明。应用不再写死 `GPIOF,9`，而是拿 `led0` alias 对应的 `gpio_dt_spec`。

### 3.2 精确工程模型

Explorer 原理图显示 LED0/LED1 和 KEY0/1/2 的实际网络；课程要求先在 schematic 中确认 MCU pin 与 active level，再写 `gpios = <&gpioX pin flags>`。`gpio-leds` / `gpio-keys` 是通用 binding；alias 是应用选取节点的稳定入口之一。

### 3.3 今天必须避免的误解

- API 名字背下来不等于理解执行路径。
- 看到一次成功输出不等于建立了可复现工程闭环。
- 教程里的地址/路径只能作为例子；板上真实值必须用工具验证。

## 4. 原理与执行路径

对每个资源执行：schematic net → MCU pin → GPIO controller/pin number → DTS child node → alias → generated data → API。不要从另一个 board DTS 直接抄。

## 5. UML / 时序

本日核心问题主要是静态结构，不强行画时序图。

## 6. References / Manuals

### Hardware/manual
- [Explorer STM32F4 schematic](../references/Explorer_STM32F4_V2.2_SCH.pdf) — p.2 MCU/pin nets, p.3 LED/KEY circuits.
- [ST RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) — GPIO chapter, only for port register semantics.

### Zephyr
- [GPIO API + Devicetree](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [`gpio-leds` binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/led/gpio-leds.html)
- [`gpio-keys` binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/input/gpio-keys.html)
- [Devicetree bindings](https://docs.zephyrproject.org/latest/build/dts/bindings.html)

Optional local ALIENTEK HAL/register guides are listed in [references/README.md](../references/README.md).

## 7. 实验准备

Open your completed `f407_board_audit.md`. Do not proceed until LED0/LED1 and KEY0/KEY1 MCU pins and active levels are confirmed from schematic.

## 8. 实验

### Lab A - LEDs
Add board-level nodes (replace GPIO/pin with your audited values):

```dts
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 { gpios = <&gpiof 9 GPIO_ACTIVE_LOW>; };
    led1: led_1 { gpios = <&gpiof 10 GPIO_ACTIVE_LOW>; };
  };
  aliases { led0 = &led0; led1 = &led1; };
};
```

Then build `samples/basic/blinky` and inspect `build/zephyr/zephyr.dts`.

### Lab B - KEY
Add `gpio-keys` nodes from audited key pins. Build a minimal app using `GPIO_DT_SPEC_GET` or an official button sample. Print press/release events.

### Evidence
```bash
grep -n -A12 -B4 'leds' build/zephyr/zephyr.dts
grep -R 'led0' build/zephyr/include/generated -n | head
```

## 9. 故障注入

- Intentionally set `GPIO_ACTIVE_HIGH` for an active-low LED and observe inverted behavior; restore.
- Temporarily remove alias and observe compile-time failure in an app that uses `DT_ALIAS(led0)`.

## 10. 调试路径

No LED → schematic active level → final `zephyr.dts` → GPIO controller status → pinctrl/clock → generated header → runtime device readiness → physical voltage with multimeter/logic analyzer.

## 11. 源码 / 系统对象追踪

Search fixed Zephyr source revision for `GPIO_DT_SPEC_GET`, `DT_ALIAS`, `gpio_pin_configure_dt`. The point is to connect macro use with generated DT artifacts, not memorize macro expansion.

## 12. 今日验收

- [ ] LED0/LED1 actual state matches DTS active flags.
- [ ] Key press is observable.
- [ ] Can find final nodes in `zephyr.dts`.
- [ ] Can explain binding vs DTS vs driver.

## 13. 面试式复述

1. `GPIO_ACTIVE_LOW` changes what?
2. alias is hardware fact or app convenience?
3. binding is code or schema?
4. why can wrong DTS compile but hardware behave wrong?
5. how does `gpio_dt_spec` avoid hard-coded board pins?

## 14. Git 交付物

board DTS diff + `led_key_test` + console log; commit `feat: add explorer led and key devicetree resources`

## 15. 明日连接

Tomorrow use west/GDB to stop inside the running Zephyr application and observe thread/register state.
