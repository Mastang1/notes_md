
**【源码与官方文档层面的证明】** 根据《Linux Device Drivers, Third Edition》（LDD3）的权威定义，整本书的**第 14 章就叫做 "The Linux Device Model"**。在这一章中，明确规定了 LDM 的四大顶级核心支柱，它们共同构建在 `kobject` 这个基础基类之上：

1. **Buses（总线）**：反映物理拓扑（如 I2C、PCIe）。
2. **Classes（设备类）**：反映逻辑业务拓扑（如 Input、TTY、Block）。
3. **Devices（设备）**：物理硬件实例。
4. **Device Drivers（驱动程序）**：软件控制逻辑。