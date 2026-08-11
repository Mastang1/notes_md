# RC 驱动 → PCI Core 心智模型（Q&A 摘要版）

> **分析对象**: `drivers/pci/controller/pcie-xilinx.c` + `drivers/pci/probe.c`（Linux 6.6.99）
> **定位**: 这是 `rc_to_PCI_core.md` 的心智模型精简版，以 Q&A 形式把"IP Vendor RC 驱动 → PCI Core"的整套逻辑压缩成一张可记忆的图。
> **适用读者**: 已读过 `rc_to_PCI_core.md`，需要一份"能复述给别人"的总结。

---

## Q1：我的综合理解

> **Question（原样引用）**：
> 我综合理解 ip vendor rc driver —— pci core 的逻辑、模式：
> RC driver 作为一个平台总线驱动，由 ip vendor 提供对应的 dts node 和 rc driver；
> 在 Linux boot 过程中执行 match，然后再 probe 中实现 rc 初始化、bridge create（bridge 作为桥接对象传递 vendor 的私有 rc 相关信息到 pci core）；
> 然后调用 `pci_host_probe(bridge)`，实现 pci core 模块的初始化，为该模块的通用 API 的内部实现提供了指针，可以把 API 的实现路由到 vendor 私有的设备信息和 apis。

---

## A1：总结回答（心智模型）

**✅ 您的理解完全正确，且与源码逐行吻合。** 把您的理解翻译成一句心智模型：

> **"Vendor 私有能力" 通过 `pci_host_bridge`（桥）被"打包 + 授权"给 PCI Core；PCI Core 从此能用统一指针（`pci_ops`）路由到 vendor 私有实现，操作任意 Endpoint——厂商差异被彻底屏蔽在 `map_bus` 一层。**

您的理解可以拆成 5 个心智节点，每个都有源码佐证：

| 心智节点 | 您的说法 | 源码落点 |
|---|---|---|
| ① 平台驱动 + DT | "RC driver 作为平台总线驱动，ip vendor 提供 dts node 和 rc driver" | `struct platform_driver xilinx_pcie_driver` + `of_match_table`（pcie-xilinx.c:608/613），DT `compatible="xlnx,axi-pcie-host-1.00.a"` |
| ② boot 中 match | "在 Linux boot 过程中执行 match" | `platform_match()` → `of_driver_match_device()`（platform.c:1335） |
| ③ probe 初始化 | "在 probe 中实现 rc 初始化" | `xilinx_pcie_parse_dt()` + `xilinx_pcie_init_port()` + `xilinx_pcie_init_irq_domain()`（pcie-xilinx.c:530/497/456） |
| ④ bridge create | "bridge 作为桥接对象传递 vendor 私有 rc 信息" | `devm_pci_alloc_host_bridge(dev, sizeof(*pcie))` → `bridge->sysdata=pcie; bridge->ops=&xilinx_pcie_ops`（pcie-xilinx.c:576/598/599） |
| ⑤ pci_host_probe 路由指针 | "实现 pci core 模块初始化，把 API 路由到 vendor 私有实现" | `pci_host_probe(bridge)`（pcie-xilinx.c:601）→ `bus->ops=bridge->ops; bus->sysdata=bridge->sysdata`（probe.c:899-900） |

**一个关键补充**（帮您把"指针路由"想得更清楚）：PCI Core 并不是"拿到 bridge 后自己抄一遍 API"，而是**把 bridge 里的指针存到每条 `pci_bus` 上**，之后每次操作 EP 时**现场回调**：

```
PCI Core 操作 EP 的配置空间
   → bus->ops->read()    [= pci_generic_config_read, access.c:80]
      → bus->ops->map_bus()  [= xilinx_pcie_map_bus, 厂商私有, pcie-xilinx.c:177]
         → bus->sysdata       [= struct xilinx_pcie *，私有信息]
         → reg_base + PCIE_ECAM_OFFSET(bus, devfn, where)
      → readl(addr)       ← 一条内存读
```

所以您的"把 API 的实现路由到 vendor 私有的设备信息和 apis"这句话，**精确对应的机制就是 `pci_ops` 函数指针 + `sysdata` 指针**这两个字段沿 `bus->sysdata/ops` 传播。


---

## Q2：`pci_host_bridge` 这个"桥接对象"到底是什么？

> **Question**：bridge 对象到底是什么？为什么叫"桥"？

**A2**：`pci_host_bridge`（include/linux/pci.h:563）是一个**同时属于两个世界的 device + 指针容器**：

```c
struct pci_host_bridge {
	struct device	dev;          // ★ 它本身是个 device（挂在 platform_device 之下）
	struct pci_bus	*bus;         // 指向根总线
	struct pci_ops	*ops;         // ★ vendor 操作接口指针（map_bus/read/write）
	struct pci_ops	*child_ops;
	void		*sysdata;     // ★ vendor 私有数据指针（struct xilinx_pcie *）
	int		busnr;
	int		domain_nr;
	struct list_head windows;     // 资源窗口
	...
	unsigned long private[];      // ★ 柔性数组：vendor 私有结构体（xilinx_pcie）就内嵌在这里
};
```

**它是"桥"的三层含义**：
1. **层级之桥**：`bridge->dev.parent = platform_device`，同时 `bus->bridge = &bridge->dev`——它把 platform 世界（父）和 PCI 世界（子）在 sysfs 树里焊在一起；
2. **接口之桥（Adapter）**：一端是 vendor 私有 API（`xilinx_pcie_ops`、irq_domain、reg_base），另一端是 PCI Core 认识的统一接口（`pci_ops` 三函数）——它把"私有方言"翻译成"通用普通话"；
3. **指针路由之桥**：`ops`（函数指针）和 `sysdata`（数据指针）这两个字段，正是您说的"把 API 路由到 vendor 私有实现"的物理载体。

> **心智锚点**：`pci_host_bridge` = **"vendor 能力注册表"**——vendor 往里填函数指针和数据指针，PCI Core 往里读，双方只通过这一个对象交换信息。

---

## Q3：`pci_host_probe(bridge)` 到底做了什么？（PCI Core 的"初始化"）

> **Question**：`pci_host_probe` 是不是"PCI Core 的初始化函数"？它具体做了哪几件事？

**A3**：是，但不完全是——它**不是全局初始化**（PCI Core 在 `pci_driver_init()` 早已注册了 `pci_bus_type`），而是**"这位 bridge 名下的 PCI 子世界"的初始化**。它做 3 件事（probe.c:3093-3123）：

```c
int pci_host_probe(struct pci_host_bridge *bridge)
{
	/* ① 注册根总线 + 递归扫描全树
	 *    pci_register_host_bridge(): 把 bridge->ops/sysdata 拷到根总线 bus->ops/sysdata
	 *    pci_scan_child_bus(): 借 pci_ops 穷举所有 devfn，实例化 pci_dev
	 */
	ret = pci_scan_root_bus_bridge(bridge);

	/* ② 资源分配：计算各桥窗口需求 → 分配 BAR/窗口 → 写进桥寄存器 */
	pci_bus_size_bridges(bus);
	pci_bus_assign_resources(bus);

	/* ③ 把 pci_dev 交给 pci_bus_type，触发 EP 驱动绑定 */
	pci_bus_add_devices(bus);
}
```

| 步骤 | 您理解中的对应 | 关键代码 |
|---|---|---|
| ① 扫描 | "pci core 模块的初始化"的第一步 | `pci_register_host_bridge()`（probe.c:880）里 `bus->ops = bridge->ops`（:900）、`bus->sysdata = bridge->sysdata`（:899） |
| ② 资源 | 为 EP 的 BAR/窗口分配地址 | `pci_bus_size_bridges/assign_resources`（setup-bus.c） |
| ③ 绑定 | EP 驱动被调用 | `pci_bus_add_devices`（bus.c:366）→ `device_attach` → `pci_bus_match` → `pci_device_probe` |

> **心智锚点**：`pci_host_probe(bridge)` = **"向 PCI Core 注册一个 vendor 子世界"**——从此 bridge 名下的总线/设备由 PCI Core 统一管理，而所有硬件操作都经 bridge 里的指针路由回 vendor。

---

## Q4：vendor 差异到底被屏蔽在哪一层？屏蔽得有多彻底？

> **Question**：厂商差异被屏蔽的"边界"在哪里？PCI Core 是否真的对 vendor 无感知？

**A4**：屏蔽边界 = **`pci_ops` 这一个结构体、`map_bus` 一个函数**。对比两家厂商即可看清：

```c
/* Xilinx（标准 ECAM）—— pcie-xilinx.c:188 */
static struct pci_ops xilinx_pcie_ops = {
	.map_bus = xilinx_pcie_map_bus,   // reg_base + ECAM_OFFSET
	.read    = pci_generic_config_read,
	.write   = pci_generic_config_write,
};

/* 假设另一家厂商（自定义寻址）—— 只需要换 map_bus 的"私有实现" */
static struct pci_ops vendor_y_ops = {
	.map_bus = vendor_y_map_bus,      // 私有的寄存器序列/地址计算
	.read    = pci_generic_config_read,  // 甚至可换成自己的 read
	.write   = pci_generic_config_write,
};
```

PCI Core 的扫描代码只写 `bus->ops->read(bus, devfn, where, size, &val)`——**它连 `struct xilinx_pcie` 是什么都不知道**。`xilinx_pcie_ops`、`xilinx_pcie_map_bus` 这些符号在 probe.c 里**根本不存在**。

**屏蔽的彻底程度**，用三个"无"概括：
- PCI Core **无** `#ifdef CONFIG_XILINX`；
- PCI Core **无** `struct xilinx_pcie` 的引用；
- PCI Core **无**任何 ECAM 地址计算代码（它只知道"问 `map_bus` 要地址"）。

> **心智锚点**：厂商差异 = `map_bus` 一个函数；`pci_ops` 就是"厂商方言 → 通用普通话"的翻译接口。**"换厂商 = 换一个 controller/pcie-xxx.c + 换一套 pci_ops"，PCI Core 一行不改。**

---

## Q5：这套设计在"设计模式"和"架构原理"里叫什么？

> **Question**：这套"vendor 私有实现 + 统一接口 + 指针路由"的设计，在软件设计上叫什么？

**A5**：它同时是 4 种经典设计模式的组合，但**主角是"策略模式"**：

| 模式 | 对应代码 | 一句话角色 |
|---|---|---|
| **策略模式（Strategy）** | `pci_ops`（map_bus/read/write）注入 | PCI Core 的扫描算法不变，换策略（vendor ops）即换厂商 |
| **适配器模式（Adapter）** | `pci_host_bridge` | 把 vendor 私有硬件接口翻译成 PCI Core 认识的标准接口 |
| **模板方法（Template Method）** | `pci_host_probe()` 三步固定骨架 | "扫描→资源→绑定"流程固定，细节由 bridge 回调提供 |
| **控制反转 / 依赖注入（IoC/DI）** | `pci_host_probe(bridge)` 之后 | 控制权归 PCI Core，vendor 驱动被"回调"而非"主动调用" |

架构原理层面：
- **开闭原则（OCP）**：加新厂商 = 加新文件 + 新 `pci_ops`，PCI Core 对扩展开放、对修改关闭；
- **依赖倒置（DIP）**：PCI Core 依赖抽象（`pci_ops` 函数指针），vendor 实现抽象，低层细节不反向依赖高层。

---

## Q6：完整心智模型（一图流）

> **Question**：能否用一张图把整套流程装进脑子？

**A6**：可以。记住这张"三层 + 两个世界 + 一个桥"的心智模型图：

```ascii
┌──────────────────────────────────────────────────────────────────────┐
│ 第一世界：Platform（固件描述，不可枚举）                                 │
│                                                                      │
│   DT node: compatible="xlnx,axi-pcie-host-1.00.a"                    │
│      │  boot 时 of_platform 创建 platform_device                     │
│      ▼                                                                │
│   platform_bus_type.match() → platform_match() → of_driver_match_device│
│      │  命中 vendor 的 of_match_table                                 │
│      ▼                                                                │
│   platform_probe() → xilinx_pcie_probe(pdev)   ★ vendor 驱动入口      │
│      │                                                                │
│      │  ① parse_dt()      ← 从 DT/device 取 reg、irq（device info）   │
│      │  ② init_port()     ← 初始化 RC 硬件                            │
│      │  ③ init_irq_domain() ← INTx/MSI 中断域                         │
│      │  ④ devm_pci_alloc_host_bridge() ← ★ 动态创建 bridge（含私有区） │
│      │  ⑤ bridge->sysdata = pcie    ← 私有数据指针                    │
│      │  ⑥ bridge->ops    = &xilinx_pcie_ops  ← 统一接口指针           │
│      ▼                                                                │
│   pci_host_probe(bridge)   ★★★★★ 跨界点/交权点                        │
└──────────────────────────────────────────────────────────────────────┘
                                     │
┌────────────────────────────────────▼──────────────────────────────────┐
│ 第二世界：PCI（可枚举，扫描发现）                                        │
│                                                                      │
│   pci_register_host_bridge()                                          │
│      ├─ bus->ops = bridge->ops       ← ★ 指针路由第一步               │
│      ├─ bus->sysdata = bridge->sysdata ← ★ 私有信息跟着走             │
│      └─ 根总线诞生（pci0000:00）                                       │
│                                                                      │
│   pci_scan_child_bus()  ★ 递归扫描                                    │
│      └─ 每个 devfn 一次配置访问：                                      │
│           bus->ops->read() → map_bus() → sysdata → reg_base+ECAM_OFFSET│
│              → readl()  ← 厂商私有寻址被完全屏蔽在这一个函数里          │
│      └─ 读到有效 VID → pci_setup_device → pci_device_add → pci_dev   │
│                                                                      │
│   pci_bus_size/assign_resources()  ← BAR/窗口分配                     │
│   pci_bus_add_devices()  ← 交给 pci_bus_type                          │
│      └─ pci_bus_match → pci_device_probe → ★ EP 驱动 probe()          │
│                                                                      │
│   Endpoint 驱动使用：BAR / IRQ(MSI via irq_domain) / DMA 全可用       │
└──────────────────────────────────────────────────────────────────────┘
```

**心智模型一句话（背下来）**：
> **"Vendor 在 Platform 世界被 LDM 启动（match→probe），把私有能力装进 `pci_host_bridge`（ops 指针 + sysdata 指针），通过 `pci_host_probe()` 交权给 PCI Core；PCI Core 借这两根指针把统一 API 路由回 vendor 实现，在 PCI 世界扫描并管理 Endpoint。"**

---

## Q7：最终总结（把您的理解浓缩成三层）

> **Question**：用最少的词，怎么给第三个人讲清楚？

**A7**：

- **第 1 层（发现）**：PCIe RC 在 Linux 里**首先是普通 SoC 外设**——vendor 提供 DT node + `platform_driver`，boot 时由 platform 总线 match 并 probe；
- **第 2 层（转化）**：probe 里 vendor 做两件事——**初始化 RC 硬件**（DT 信息→寄存器/中断/MSI 域），**创建 `pci_host_bridge` 并填入接口指针**（`ops`/`sysdata`）；
- **第 3 层（交权）**：`pci_host_probe(bridge)` 把指针交进 PCI Core——Core 把 `ops/sysdata` 存到每级 `pci_bus` 上，之后**用统一接口 `bus->ops` 路由回 vendor 实现**，扫描出 Endpoint、分配资源、绑定驱动。

**您的原话的"最大正确之处"**：您抓住了"**指针路由**"这个本质——`pci_host_bridge` 不是"数据拷贝"，而是"**函数指针 + 数据指针的注册与传播**"，PCI Core 的一切统一 API 最终都经由这两根指针落回 vendor 私有实现。这就是整个架构的心智内核。

---

## 附录：核心代码-概念映射速查

| 您的概念 | 代码符号 | 文件:行 |
|---|---|---|
| vendor DT node | `compatible="xlnx,axi-pcie-host-1.00.a"` | 板级 dts |
| vendor 平台驱动 | `xilinx_pcie_driver` / `builtin_platform_driver` | pcie-xilinx.c:613/621 |
| match | `platform_match` → `of_driver_match_device` | platform.c:1335 |
| probe 入口 | `xilinx_pcie_probe` | pcie-xilinx.c:566 |
| device info 消费 | `xilinx_pcie_parse_dt` / `init_port` / `init_irq_domain` | pcie-xilinx.c:530/497/456 |
| bridge create | `devm_pci_alloc_host_bridge(dev, sizeof(*pcie))` | probe.c:623 |
| 私有区 | `pci_host_bridge_priv` / `private[]` | pci.h:602 |
| 接口指针 | `bridge->ops = &xilinx_pcie_ops` | pcie-xilinx.c:599 |
| 数据指针 | `bridge->sysdata = pcie` | pcie-xilinx.c:598 |
| 交权 | `pci_host_probe(bridge)` | pcie-xilinx.c:601 |
| 指针路由落地 | `bus->ops = bridge->ops; bus->sysdata = bridge->sysdata` | probe.c:899-900 |
| 统一 API 回路由 | `bus->ops->map_bus` → `xilinx_pcie_map_bus` | pcie-xilinx.c:177 |
| ECAM 换算 | `PCIE_ECAM_OFFSET(bus, devfn, where)` | pci-ecam.h:34 |
