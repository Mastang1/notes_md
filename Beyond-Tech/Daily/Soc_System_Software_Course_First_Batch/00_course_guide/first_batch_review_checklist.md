# 第一批教程用户验收清单

请优先看 `W01D01` 与 `A01_DeviceTree`，再随机抽一天。

- [ ] W01D01 是否已经是“照着能做”的教程，而不是目标清单？
- [ ] 每日核心主动学习量是否约 2 小时可完成？
- [ ] Feynman 白话模型是否帮助理解，而不是降低技术精度？
- [ ] Mermaid 图是否说明系统关系，而不是装饰？
- [ ] UML 只在真正存在调用/启动时序时出现？
- [ ] 本地 Explorer PDF 页码/链接是否好用？
- [ ] i.MX6ULL 未上传 PDF 的位置是否诚实写“章节已核实、页码未伪造”？
- [ ] DeviceTree 是否真正讲清 `DTS → DTB → device_node → bus-specific object → struct device → match/probe`？
- [ ] 是否讲清“不是所有 DT node 都是 platform_device”？
- [ ] 是否讲清 I2C/SPI/PCIe 的 device object 差异？
- [ ] Linux DT 与 Zephyr DT 构建/运行时差异是否清晰？
- [ ] 哪些部分太啰嗦？哪些地方仍然跳步？

## 建议先看的 3 个文件

1. `02_linux/week01/W01D01_Ubuntu_Host_Environment.md` - 检查“每日教程”是否足够可执行；
2. `03_zephyr/week01/W01D06_STM32F407_Hardware_Audit_and_Board_Port_Preparation.md` - 检查是否真正结合你的 Explorer 硬件；
3. `04_deep_dive/A01_DeviceTree_From_DTS_to_Linux_Device.md` - 检查难点讲解深度。

质量复核记录：`first_batch_quality_review.md`。
