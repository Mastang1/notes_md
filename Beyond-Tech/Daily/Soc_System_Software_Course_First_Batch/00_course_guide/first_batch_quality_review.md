# 第一批教程质量复核报告

## 1. 复核范围

本轮复核覆盖：

- 课程入口与 reference 体系；
- Week 1 七份 Daily Tutorial；
- Explorer STM32F407 硬件审计模板；
- `A01_DeviceTree_从DTS到LinuxDevice.md`；
- Mermaid/UML、相对链接、source ID、每日 2h 预算、硬件事实和代码版本兼容性。

## 2. 已完成的规格符合性检查

| 检查项 | 结果 |
|---|---|
| Week 1 Daily Tutorial 数量 | 7 / 7 |
| Daily 强制章节 | PASS |
| 每日约 2h 主动学习预算 | PASS |
| DeviceTree 必需执行链 | PASS |
| DeviceTree Mermaid >= 3 | PASS |
| DeviceTree UML sequenceDiagram | PASS |
| Source ID 未注册引用 | 0 |
| 内部 Markdown 链接错误 | 0 |
| Explorer 本地 PDF 链接错误 | 0 |
| 未完成内容占位符 | 0 |
| 错误 MCU 型号假设 | 0 |

自动检查明细见 `validation_report.json`。

## 3. 硬件事实复核与修正

### 3.1 MCU exact part

Explorer 原理图 p.2 的 U4 为 `STM32F407ZET6`。进一步以 ST DS8626 Rev 12 核实：

- `Z`：144 pins；
- `E`：512 KB internal Flash；
- `T`：LQFP；
- `6`：-40~85 °C；
- system SRAM：192 KB（112+16+64 KB CCM）；
- backup SRAM：4 KB。

因此课程不再沿用 Discovery `STM32F407VG/1 MB` 的板级假设。

### 3.2 USB-UART 链路

重新按原理图 p.2/p.4 核查后补充：

```text
PC USB
→ CH340G
→ TXD/RXD
→ P6 2x2 jumper/header
→ PA10/USART1_RX, PA9/USART1_TX
→ USART1
```

因此 Zephyr console bring-up 前必须检查 P6 跳帽，而不能仅靠“板上有 CH340”推断 MCU UART 已连通。

### 3.3 LED / KEY 极性

根据 p.3 电路：

- LED0/PF9、LED1/PF10：GPIO 灌电流，active-low；
- KEY0/PE4、KEY1/PE3、KEY2/PE2：按键闭合到 GND，active-low。

这些极性已进入 hardware inventory 和 board audit，不从示例代码猜测。

### 3.4 W25Q128

原理图确认 W25Q128 + SPI1；Winbond 官方资料确认 W25Q128 属于 128 Mbit（16 MiB）容量等级。由于原理图未给完整器件后缀，erase/program geometry 暂不锁死，后续 MCUboot 阶段必须读取实物丝印后匹配对应 datasheet。

## 4. 教程工程性修正

### 4.1 网络故障注入

原来的“把 /24 改 /16”不能保证故障，因为扩大 prefix 后对端仍可能被判断为直连。已改为明确切换到另一个 `/24` 网段，通过串口恢复。

### 4.2 Git 交付

将 `git commit -am` 改为 `git add . && git commit`，避免新生成的实验文件未被 commit。

### 4.3 i.MX6ULL 旧 BSP Kernel API

DeviceTree `student,mydev` 实验优先兼容常见老版本 Linux 4.x BSP：

- 使用 `dev_err()` 而不是依赖较新内核便利 API；
- 示例 `remove()` 使用旧 BSP 常见的 `int` callback；
- 文档要求实际开发时以目标 Kernel 的 `struct platform_driver` 头文件为准。

## 5. DeviceTree 专题复核

专题已覆盖以下主链：

```text
DTS / DTSI
→ dtc
→ DTB/FDT
→ U-Boot handoff
→ early DT scan
→ unflatten_device_tree()
→ struct device_node
→ bus/subsystem population
→ platform_device / i2c_client / spi_device / pci_dev
→ common struct device / driver model
→ bus match
→ of_match_table / compatible
→ probe()
```

并单独回答：

- 为什么不是所有 node 都变成 `platform_device`；
- `struct device` 的“统一”来自哪里；
- I2C/SPI/PCIe 的对象为什么不同；
- DTS grammar、binding schema、driver implementation 为什么不是同一件事；
- 不同厂商 DTS 为什么看起来不同；
- Linux DeviceTree runtime population 与 Zephyr build-time Devicetree 的本质差异。

## 6. 已知边界

当前会话实际上传的是 Explorer F4 原理图。截图中列出的两本 i.MX6ULL 大 PDF 尚未作为附件上传，因此本批：

- 不伪造 PDF 精确页码；
- 使用公开的 正点原子 `imx6ull-document` 仓库和已核实章节名；
- `references/README.md` 预留了本地相对路径；
- 用户把同名 PDF 放入 `references/` 后，后续批次可以进一步补精确 `#page=` 锚点。

## 7. 第一批结论

本批达到方案 C 的评审条件，可以交付用户实际执行 Week 1，并用 `A01_DeviceTree` 评估教程深度与组织方式。用户验收后再按同一模板展开 Week 2-5。
