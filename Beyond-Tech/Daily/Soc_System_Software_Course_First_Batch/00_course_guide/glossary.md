# Glossary

- **Host**：执行编辑、编译、调试工具的开发机。本课程主要是 Ubuntu VM。
- **Target**：最终运行程序的硬件平台，例如 i.MX6ULL、STM32F407。
- **BSP**：Board Support Package。连接 SoC、板级硬件与 OS 的启动、配置和驱动集合。
- **Toolchain**：compiler/assembler/linker/binutils 等工具集合。
- **Cross compiler**：在 Host 上产生另一 ISA/ABI Target 代码的编译器。
- **Sysroot**：模拟 Target 根目录中头文件和库布局的编译/链接视图。
- **ELF section**：链接期视角的逻辑内容分组，如 `.text/.data/.bss/.symtab`。
- **ELF segment**：loader 视角的加载区域，由 Program Header 描述，通常一个 segment 包含多个 section。
- **Device Tree / DTS**：以树形数据描述硬件拓扑与资源的源文本。
- **DTB/FDT**：DTS 经 `dtc` 编译后的扁平二进制表示；FDT 强调数据格式，DTB 强调文件产物。
- **`struct device_node`**：Linux 把 FDT unflatten 后，在内存中用于表示 DT 节点的 Kernel 对象。
- **`struct device`**：Linux Driver Model 的公共设备对象，用于 bus/driver/class/PM/sysfs 等统一机制。
- **`platform_device`**：SoC/板载、CPU 可直接寻址且没有 PCI/USB 这类自描述枚举协议的常见设备 wrapper，内部包含 `struct device dev`。
- **`platform_driver`**：platform bus 上的驱动对象，提供 `probe/remove` 等。
- **probe**：device 与 driver 匹配后，由 driver core/bus 流程触发驱动初始化入口，不是应用程序直接调用。
- **Kconfig**：控制软件功能“编不编、以何种配置编”的配置系统。
- **west**：Zephyr 的 meta-tool，管理 workspace、manifest projects、build/flash/debug extension commands。
- **Zephyr binding**：YAML 文件，描述一个 `compatible` 对应节点有哪些属性、类型、约束，以及如何产生生成信息。
