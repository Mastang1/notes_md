---
tags:
  - OS
  - Linux
---

# PCIe 硬件心智模型（阶段 0：由外到内）

> **目的**: 为学习 `drivers/pci/`（尤其 `pcie-xilinx.c`）建立坚实的**硬件底层心智模型**。
> **学习方法**: 每层按"**它是什么 → 它解决什么问题 → 一张图记住**"；全部内容与 Linux 6.6.99 源码 + PCIe 官方规范/Wikipedia 对照。
> **适用**: 已了解 RC 驱动 → PCI Core 软件链路，但缺硬件组成/机制底子的读者。

---

## 目录

1. [0.1 拓扑组成——PCIe 是一棵树](#01-拓扑组成pcie-是一棵树)
2. [0.2 分层协议与 TLP——数据怎么走](#02-分层协议与-tlp数据怎么走)
3. [0.3 配置空间——软件如何发现硬件](#03-配置空间软件如何发现硬件)
4. [0.4 地址空间与 BAR——地址怎么分配与译码](#04-地址空间与-bar地址怎么分配与译码)
5. [0.5 中断——设备如何通知 CPU](#05-中断设备如何通知-cpu)
6. [心智模型总览（一图流）](#06-心智模型总览一图流)
7. [参考教程与代码锚点](#07-参考教程与代码锚点)

---

## 0.1 拓扑组成——PCIe 是一棵树

### 它是什么

PCI Express 是一个**树形拓扑的点对点串行互连**（替代旧 PCI 的共享并行总线）。树上每个节点都有自己的角色：

| 组成 | 角色 | 说明 |
|---|---|---|
| **Root Complex (RC)** | 树根，CPU 侧 | 把 CPU 的读写/中断请求翻译成 PCIe 事务；每个 RC 对应一个 PCI 域（domain） |
| **Root Port** | RC 伸出的"根枝" | 每条根端口背后是一条**根总线**（bus 0）；Xilinx AXI PCIe IP 就是 RC + 单根端口 |
| **Switch** | 树的"分叉" | 由 **1 个 Upstream Port（面向 RC）** + **N 个 Downstream Port（面向下游）** 组成，把一条链路扩展成多条 |
| **Endpoint (EP)** | 树的"叶子" | 真正干活的设备（网卡/NVMe/GPU），一个功能（function）即一个可寻址对象 |
| **PCIe-to-PCI Bridge** | 兼容旧设备 | 把 PCIe 树延伸到传统 PCI 总线 |

### 它解决什么问题

旧 PCI 是共享总线（所有人抢一条线），扩展性差、无热插拔。PCIe 用**点对点串行链路**（一对差分收发线 = 1 lane，x1/x4/x8/x16 组合）+ **交换机**自由扩展成树。

### 心智模型图

```
CPU
 └─ Root Complex (RC)                      <- Linux: pci_host_bridge
     ├─ Root Port 0 ── Bus 0 ── [EP]       <- Linux: root pci_bus
     └─ Root Port 1 ── Bus 0 ── Switch
                             ├─ Upstream Port
                             ├─ Downstream Port A ── Bus 1 ── [EP]
                             └─ Downstream Port B ── Bus 2 ── [EP] ── [EP]
```
- 每个"枝/叶"在 Linux 里是一个 `pci_bus`（枝）或 `pci_dev`（叶）；
- `pci_scan_child_bus()` 递归遍历的就是这棵树（软件镜像硬件拓扑）。

### 与源码的对应

| 硬件 | Linux 软件对象 | 代码 |
|---|---|---|
| RC + Root Port | `struct pci_host_bridge` + 根 `pci_bus` | `pci_register_host_bridge()`（probe.c:880）|
| 任意节点设备 | `struct pci_dev`（含 `bus`/`subordinate`） | `pci_scan_device()`（probe.c:2447）|
| 链路状态 | Xilinx `PSCR` 的 `LNKUP` 位 | pcie-xilinx.c:88 / 123-127 |

> **参考**: Wikipedia《PCI Express》§Architecture；内核 `Documentation/driver-api/pci/pci.html`。

**验收**：能画出 RC→Switch→EP 的树，并说出每个节点在 Linux 里是 `pci_bus` 还是 `pci_dev`。

---

## 0.2 分层协议与 TLP——数据怎么走

### 它是什么

PCIe 是一个**三层分层协议**（仿 OSI 思想，层间只通过标准接口交互）：

| 层 | 职责 | 类比 |
|---|---|---|
| **事务层 (Transaction)** | 产生/解析 **TLP**（事务层包）；区分完成/非完成事务；流控 | 快递面单：寄什么、寄给谁 |
| **数据链路层 (Data Link)** | 给 TLP 加 Sequence Number + LCRC 校验；ACK/NAK 重传；流量控制 | 快递运输途中的核对 |
| **物理层 (Physical)** | 串行差分信号、链路训练、加扰、时钟恢复 | 快递实际跑路的货车 |

### TLP 是核心概念

软件每一次操作（读配置/读内存/发中断）最终都变成链路上的一个 **TLP**。TLP 头大致为：

```
[ Header: Type(读写/配置/消息) + 地址 或 RequesterID(Bus:Dev:Fnc) + Tag... ] [ Data ] [ Digest ]
```

常用 TLP 类型（对应 Linux 中的操作）：

| TLP 类型 | 用途 | Linux 侧操作 |
|---|---|---|
| `CfgRd0/1` / `CfgWr0/1` | 配置空间读/写（枚举阶段用） | `pci_bus_read_config_*` |
| `MRd` / `MWr` | 内存读/写（数据与 MMIO） | `readl()` / `writel()` |
| `IORd` / `IOWr` | I/O 空间读/写 | `inb()` / `outb()` |
| `Msg` | 消息（MSI 中断、PME 等） | MSI 中断 |

### 心智模型

> **软件每次"读寄存器"最终都变成一个 TLP 包，在链路上从 RC 路由到目标 BDF 设备；设备响应后返回一个完成 TLP（Completion）**。

### 与源码的对应（关键：ECAM 为什么是"读内存"）

在 **ECAM**（增强配置访问机制）下，"发一个 CfgRd TLP"被硬件简化为"读一段被映射的内存"：

```c
// pcie-xilinx.c:177 —— 把 BDF+offset 算成内存地址
static void __iomem *xilinx_pcie_map_bus(struct pci_bus *bus,
					 unsigned int devfn, int where)
{
	...
	return pcie->reg_base + PCIE_ECAM_OFFSET(bus->number, devfn, where);
}

// access.c:80 —— 然后就是一条普通内存读
int pci_generic_config_read(struct pci_bus *bus, unsigned int devfn,
			    int where, int size, u32 *val)
{
	addr = bus->ops->map_bus(bus, devfn, where);
	...
	*val = readl(addr);   // 内存读触发 RC 内部产生 CfgRd TLP
	return PCIBIOS_SUCCESSFUL;
}
```

**底层发生了什么**：CPU `readl()` → 访问 ECAM 窗口内存 → RC 的硬件逻辑识别该地址落在配置窗口内 → 构造 `CfgRd0 TLP`（含 BDF）→ 经 Switch 路由到目标 EP → EP 返回完成 TLP → RC 把数据送回 `readl` 返回值。

> **参考**: Wikipedia《PCI Express》§Hardware protocol summary（三层协议 + TLP）。

**验收**：能讲清"软件读 Vendor ID → 内存读 → 底层变成 CfgRd TLP 到 BDF 设备 → 完成 TLP 返回数据"整条链。
---

## 0.3 配置空间——软件如何发现硬件

### 它是什么

每个 PCI 功能（function）都有一块 **配置空间（Configuration Space）**，软件通过它"认识"和"配置"设备：

| 概念 | 内容 |
|---|---|
| **BDF 地址** | Bus(8bit) : Device(5bit) : Function(3bit)，最大 256 总线 x 32 设备 x 8 功能 |
| **标准配置空间** | 每个功能 256 字节，前 64 字节标准化；其余厂商自定义 |
| **PCIe 扩展配置空间** | 每个功能 4KB（ECAM 模式），低 256B 与标准重叠 |
| **能力链表 (Capability List)** | 0x34 处指向第一个 Capability，后续设备特性（PM/MSI/PCIe/AER...）都在链表上 |

### 标准配置空间头（前 64 字节，来自 include/uapi/linux/pci_regs.h）

```c
#define PCI_STD_HEADER_SIZEOF	64      /* 标准头大小 */
#define PCI_STD_NUM_BARS	6       /* 标准 BAR 数量 */
#define PCI_VENDOR_ID		0x00    /* 16 位：厂商 ID（0xFFFF=空槽） */
#define PCI_DEVICE_ID		0x02    /* 16 位：设备 ID */
#define PCI_COMMAND		0x04    /* 命令寄存器 */
#define  PCI_COMMAND_IO		0x1     /* 使能 I/O 空间响应 */
#define  PCI_COMMAND_MEMORY	0x2     /* 使能内存空间响应 */
#define  PCI_COMMAND_MASTER	0x4     /* 使能总线主控（DMA） */
#define PCI_STATUS		0x06    /* 状态寄存器 */
#define PCI_CLASS_REVISION	0x08    /* 高 24 位 class，低 8 位 revision */
#define PCI_HEADER_TYPE		0x0e    /* 头类型：0=普通 1=桥 2=CardBus */
#define PCI_BASE_ADDRESS_0	0x10    /* BAR0 ~ BAR5（0x10~0x24） */
#define PCI_CAPABILITY_LIST	0x34    /* 第一个 Capability 偏移 */
#define PCI_INTERRUPT_LINE	0x3c    /* 中断线 */
#define PCI_INTERRUPT_PIN	0x3d    /* 中断引脚 */
```

### ECAM 寻址公式（把 BDF 线性映射到内存）

```c
// include/linux/pci-ecam.h
#define PCIE_ECAM_BUS_SHIFT	20  /* Bus 占 bit20~27 */
#define PCIE_ECAM_DEVFN_SHIFT	12  /* Device+Function 占 bit12~19 */
#define PCIE_ECAM_OFFSET(bus, devfn, where) \
	(PCIE_ECAM_BUS(bus) | PCIE_ECAM_DEVFN(devfn) | PCIE_ECAM_REG(where))
```

总窗口大小：256 总线 x 32 设备 x 8 功能 x 4KB = **256MB** 线性地址空间，每个功能一个 4KB 页。

### 心智模型

> **配置空间 = 每个设备的"户口本"；BDF 是"门牌号"；枚举 = 挨家挨户读 0x00 的 Vendor ID，非 0xFFFFFFFF 就是有人住**。

### 与源码的对应

| 硬件概念 | 软件代码 | 位置 |
|---|---|---|
| 读 Vendor ID 判定设备存在 | `pci_bus_read_dev_vendor_id()` | probe.c:2424 |
| 解析配置头（class/BAR/IRQ） | `pci_setup_device()` | probe.c:1853 |
| 按头类型区分设备/桥 | `PCI_HEADER_TYPE_*`（0/1/2） | pci_regs.h:80-82 |
| ECAM 偏移计算 | `PCIE_ECAM_OFFSET` | pci-ecam.h:34 |
| 能力探测 | `pci_find_capability()` / `pci_init_capabilities()` | probe.c:2491 |

> **参考**: Wikipedia《PCI configuration space》§Overview / §Standardized registers / §ECAM。

**验收**：能默写出配置空间头关键字段偏移（VID=0、CMD=4、BAR0=0x10、HeaderType=0xE），并说出 ECAM 公式与 256MB 窗口的来历。
---

## 0.4 地址空间与 BAR——地址怎么分配与译码

### 它是什么

PCI/PCIe 设备有**三种地址空间**，软件通过不同方式访问：

| 地址空间 | 用途 | 访问方式 |
|---|---|---|
| **内存空间 (MEM)** | 设备寄存器/缓冲区，可被 CPU 直接访问 | `readl()`/`writel()`（MMIO） |
| **I/O 空间 (IO)** | 旧式端口，x86 特化 | `inb()`/`outb()` |
| **配置空间 (CFG)** | 设备"户口本"（见 0.3） | ECAM / 配置端口 |

**BAR（Base Address Register）** 是设备向软件"申请地址空间"的寄存器——设备在 BAR 里声明"我要一块多大的地址窗口"，软件分配后把基址写进去，从此 CPU 读写该窗口即访问设备。

### BAR 的位编码（来自 pci_regs.h）

```c
#define PCI_BASE_ADDRESS_SPACE		0x01  /* bit0: 0=内存, 1=I/O */
#define PCI_BASE_ADDRESS_SPACE_MEMORY	0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_64	0x04  /* bit2: 64 位地址 */
#define PCI_BASE_ADDRESS_MEM_PREFETCH	0x08  /* bit3: 可预取 */
#define PCI_BASE_ADDRESS_MEM_MASK	(~0x0fUL) /* 内存 BAR 有效位掩码 */
#define PCI_BASE_ADDRESS_IO_MASK	(~0x03UL)  /* I/O BAR 有效位掩码 */
```

- **bit0** = 空间类型：0→内存空间，1→I/O 空间；
- 内存 BAR：**bit1~bit2**=MEM_TYPE（bit2=1 即 64 位地址），**bit3**=可预取标志；
- 其余高位是**基址**，且**低 n 位只读为 0**——这 n 个 0 就是设备申请的大小（对齐）的编码。

### BAR 大小探测原理（"写 1 读回"）

pci_regs.h 官方注释原文：

```
Base addresses specify locations in memory or I/O space.
Decoded size can be determined by writing a value of 0xffffffff
 to the register, and reading it back.  Only 1 bits are decoded.
```

算法（对应 `pci_read_bases()` / `pci_size()`，probe.c:110/321）：

```
1. 保存 BAR 原值
2. 向 BAR 写 0xFFFFFFFF
3. 读回 -> 低 n 位为 0  =>  大小 = 2^n（对齐）
   例：读回 0xFFFFF000 -> 低 12 位为 0 -> 大小 4KB，基址需 4KB 对齐
4. 恢复原值
5. 按 bit0（空间）、bit1~bit2（MEM_TYPE/64位）、bit3（prefetch）判定类型
```

### 地址译码（设备侧）

设备内部比较"链路总线地址是否落在自己某个 BAR 窗口内"：命中则响应读写（MEM），未命中则忽略。这就是**为什么扫描时能区分设备**——空槽读返回 0xFFFFFFFF（主设备中止），有设备则返回其 Vendor ID。

### 心智模型

> **BAR = 设备给软件填写的"地址空间申请单"**：枚举先"量尺寸"（写 1 读回），分配时软件把一段地址写进 BAR；此后 CPU 读写这段地址 = 访问设备寄存器。

### 与源码的对应

| 硬件概念 | 软件代码 | 位置 |
|---|---|---|
| 量 BAR 尺寸 | `pci_read_bases()` / `pci_size()` | probe.c:321 / 110 |
| 给 BAR 分配地址 | `__pci_assign_resource()` | setup-res.c |
| 桥窗口需求计算 | `pci_bus_size_bridges()` | setup-bus.c:1323 |
| 写桥 Base/Limit 寄存器 | `pci_setup_bridge_mmio()` 等 | setup-bus.c:571/608/627 |
| 地址翻译（CPU vs 总线） | `pcibios_resource_to_bus()` / `bus_to_resource()` | host-bridge.c |

> **参考**: Wikipedia《PCI configuration space》§Base Address Registers + §ECAM。

**验收**：能用"写 0xFFFFFFFF 读回"讲清 BAR 大小探测；说出 BAR bit0/bit1/bit3 含义；理解"空槽读回 0xFFFFFFFF"的原理。
---

## 0.5 中断——设备如何通知 CPU

### 它是什么

PCIe 设备通知 CPU 有三种方式：

| 类型 | 机制 | 特点 |
|---|---|---|
| **INTx（Legacy）** | 4 根共享中断线 INTA~INTD（引脚方式），PCIe 用 Msg TLP 模拟 | 旧式、共享、需路由仲裁 |
| **MSI** | 设备向**指定内存地址**发一次内存写（MWr TLP），RC 据地址/数据译出中断号 | 无需引脚；向量数 1/2/4/8/16/32 |
| **MSI-X** | 同 MSI 思路，但用**独立表项**（每项含地址+数据），可更多更灵活 | 现代设备首选（NVMe/网卡） |

### MSI 的精髓（一句话）

> **"设备向一个特定内存地址写一个数据"，就是"发一个中断"**。

流程：

```
1. 软件（驱动/内核）把 MSI 目标地址 + 数据配置进设备的能力结构（Capability）
2. 设备产生中断时，构造一个内存写（MWr）TLP，发往该地址
3. RC 收到该写事务，按地址/数据路由到 CPU 中断控制器（如 GIC/APIC）
4. CPU 触发对应中断向量，执行 ISR
```

注意：MSI 的"数据"只用来选中断向量，**不携带业务数据**（常见误解）。

### MSI 与 INTx 的对比

| | INTx | MSI/MSI-X |
|---|---|---|
| 物理形态 | 引脚（PCIe 用消息模拟） | 内存写事务 |
| 数量 | 每设备 4 个（共享） | MSI 最多 32，MSI-X 可达几千 |
| 路由开销 | 需中断控制器仲裁共享 | 直接路由，性能好 |
| 现代设备 | 兼容性保留 | 首选 |

### 心智模型

> **中断 = 设备向内存"写"一下**。RC 的 MSI 寄存器（如 Xilinx 的 MSIBASE1/2）就是告诉设备"往哪个地址写"的"信箱"。

### 与源码的对应（Xilinx RC）

| 硬件概念 | 软件代码 | 位置 |
|---|---|---|
| 把 MSI 目标地址写入 RC 寄存器 | `pcie_write(pcie, ..., MSIBASE1/2)` | pcie-xilinx.c:486-487 |
| MSI 地址/数据编制 | `xilinx_compose_msi_msg()` | pcie-xilinx.c:216 |
| MSI 中断域 | `xilinx_allocate_msi_domains()` | pcie-xilinx.c:482 |
| INTx 中断域 | `irq_domain_add_linear(..., PCI_NUM_INTX, ...)` | pcie-xilinx.c:469 |
| 中断解码（INTx vs MSI） | `xilinx_pcie_intr_handler()` 读 RPIFR1 | pcie-xilinx.c:387-414 |
| EP 申请中断 | `pci_alloc_irq_vectors()` | drivers/pci/msi/api.c |

> **参考**: Wikipedia《Message Signaled Interrupts》§Overview / §Advantages / §MSI types。

**验收**：能讲清"设备发 MSI 中断 = 向某内存地址写一个数据"，并解释为什么 RC 驱动要写 MSIBASE 寄存器、为什么 `xilinx_pcie_intr_handler` 要读 RPIFR1 区分 INTx/MSI。
---

## 0.6 心智模型总览（一图流）

把 0.1~0.5 五层压缩成一张图：

```
        CPU
         │
   ┌─────▼─────┐   0.1 拓扑：RC 是树根
   │ RootComplex │
   └─────┬─────┘
         │ Root Port（Bus 0）
   ┌─────▼─────┐   Switch 扩展：Downstream Port → Bus 1/2...
   │  Switch    │
   └──┬───┬───┘
      │   └────── [EP]（叶子）
      ▼
    [EP]

   每步操作（0.2）：软件调用 → TLP 包（CfgRd/MRd/MWr/Msg）→ 经三层流水线 → BDF 路由
   设备身份（0.3）：配置空间 = 户口本；BDF = 门牌号；Vendor ID 非 0xFFFF = 存在
   地址窗口（0.4）：BAR = 申请单；写 0xFFFFFFFF 读回 = 量尺寸；分配后写基址
   事件通知（0.5）：中断 = 设备向内存"写"一下（MSI）；RC 按地址/数据路由到 CPU
```

### 一句话贯穿

> **PCIe 的真相是"一切皆内存写"**：读配置、读 MMIO、发中断，最终都化为带地址的 TLP，在树形拓扑里被 BDF/地址路由。软件层（Linux `pci_*` API）负责把这些翻译成对 RC 寄存器/内存的操作。

### 学习检查表

| # | 能力 | 对应章节 | 自评 |
|---|---|---|---|
| 1 | 画出 RC→Switch→EP 树并对应 `pci_bus`/`pci_dev` | 0.1 | |
| 2 | 讲清 TLP 与三层协议、ECAM 为何是内存读 | 0.2 | |
| 3 | 默写配置头关键偏移、ECAM 公式 | 0.3 | |
| 4 | 讲清 BAR 量尺寸与地址译码 | 0.4 | |
| 5 | 讲清 MSI=内存写、INTx 对比 | 0.5 | |

---

## 0.7 参考教程与代码锚点

### 网络/官方教程

| 教程 | 覆盖 | 链接 |
|---|---|---|
| Wikipedia《PCI Express》 | 拓扑、三层协议、TLP | https://en.wikipedia.org/wiki/PCI_Express |
| Wikipedia《PCI configuration space》 | 配置空间、BDF、ECAM、BAR | https://en.wikipedia.org/wiki/PCI_configuration_space |
| Wikipedia《Message Signaled Interrupts》 | MSI/MSI-X 原理 | https://en.wikipedia.org/wiki/Message_Signaled_Interrupts |
| 内核《How To Write Linux PCI Drivers》 | 驱动编写流程 | https://docs.kernel.org/driver-api/pci/pci.html |
| 内核《PCI Test User Guide》 | EP/RC 实践 | https://docs.kernel.org/PCI/endpoint/pci-test-howto.html |

### 本仓库代码锚点（Linux 6.6.99）

| 文件 | 关键内容 |
|---|---|
| `drivers/pci/controller/pcie-xilinx.c` | Xilinx RC 驱动（寄存器/IP 操作） |
| `drivers/pci/probe.c` | 扫描、`pci_setup_device`、`pci_size` |
| `drivers/pci/access.c` | `pci_generic_config_read/write` |
| `drivers/pci/pci-ecam.h`（include/linux/） | `PCIE_ECAM_OFFSET` 公式 |
| `include/uapi/linux/pci_regs.h` | 配置空间寄存器偏移定义 |
| `drivers/pci/setup-bus.c` / `setup-res.c` | 资源分配 |
| `drivers/pci/msi/` | MSI 子系统 |

> **下一步（阶段 1）**：把本模型的硬件概念逐一对到 `pcie-xilinx.c` 的寄存器定义，形成"寄存器 ↔ 硬件概念 ↔ 软件行为"映射。