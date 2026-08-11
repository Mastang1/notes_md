# Xilinx AXI PCIe IP 硬件-软件映射（阶段 1）

> **目的**: 把阶段 0 的 PCIe 硬件心智模型，**逐一对到 `drivers/pci/controller/pcie-xilinx.c` 的寄存器与软件行为**，完成"由外到内"的最后一步。
> **对象**: Xilinx AXI PCIe Bridge（`compatible = "xlnx,axi-pcie-host-1.00.a"`），Linux 6.6.99。
> **方法**: 三栏映射——**寄存器（硬件） ↔ 硬件概念（阶段0） ↔ 软件行为（pcie-xilinx.c 代码）**。

---

## 目录

1. [1.1 IP 概述：这个 IP 在 PCIe 树中的位置](#11-ip-概述这个-ip-在-pcie-树中的位置)
2. [1.2 寄存器地图总览](#12-寄存器地图总览)
3. [1.3 三栏映射表（核心）](#13-三栏映射表核心)
4. [1.4 配置访问回路（寄存器层面）](#14-配置访问回路寄存器层面)
5. [1.5 中断完整回路（寄存器层面）](#15-中断完整回路寄存器层面)
6. [1.6 链路管理与 Bridge Enable](#16-链路管理与-bridge-enable)
7. [1.7 MSI 目标地址机制](#17-msi-目标地址机制)
8. [1.8 心智模型总结](#18-心智模型总结)
9. [参考](#19-参考)

---

## 1.1 IP 概述：这个 IP 在 PCIe 树中的位置

```
 CPU + AXI 总线（Zynq/ZynqMP PS 侧）
 │
 ▼
 Xilinx AXI PCIe Bridge IP  =  Root Complex + 1 个 Root Port
 │
 │ 配置窗口（reg_base，ECAM 内存映射）
 ▼
 PCIe 链路（x1/x4...）
 │
 └── [Endpoint]（下游设备，如 NVMe/网卡）
```

- 它是 **RC + 单根端口**：根总线 bus 0 上只允许一个设备（`devfn==0`），代码里 `xilinx_pcie_valid_device()` 强制了这一约束；
- 它遵循**标准 ECAM**：配置空间用内存映射方式暴露，因此 read/write 可复用通用 `pci_generic_config_read/write`，只需自定义 `map_bus`；
- 它把 AXI 侧中断（IDR/IMR）与 PCIe 侧事件（INTx/MSI/错误）做**桥接翻译**。

### 与阶段 0 的对应

| 阶段 0 概念 | 本 IP 中的体现 |
|---|---|
| 0.1 拓扑 | RC + 单 Root Port；`pci_is_root_bus` 判断根/下游 |
| 0.2 分层/TLP | 配置访问走 ECAM 内存读；中断走 Msg TLP |
| 0.3 配置空间 | `reg_base + PCIE_ECAM_OFFSET` 暴露 4KB/功能 |
| 0.4 BAR | RC 自己的配置窗口就是一个"BAR"；EP 的 BAR 由 PCI Core 分配 |
| 0.5 中断 | `leg_domain`（INTx）+ `msi_domain`（MSI）两级路由 |
---

## 1.2 寄存器地图总览

`pcie-xilinx.c` 头部定义的寄存器（偏移相对 `reg_base`）：

| 偏移 | 宏名 | 功能分类 |
|---|---|---|
| 0x130 | `XILINX_PCIE_REG_BIR` | Bridge Info（ECAM 大小信息） |
| 0x138 | `XILINX_PCIE_REG_IDR` | 中断解码寄存器（哪个中断发生了） |
| 0x13c | `XILINX_PCIE_REG_IMR` | 中断屏蔽寄存器（使能/屏蔽哪些） |
| 0x144 | `XILINX_PCIE_REG_PSCR` | Phy 状态/控制（链路是否 UP） |
| 0x148 | `XILINX_PCIE_REG_RPSC` | Root Port 状态/控制（Bridge Enable） |
| 0x14c | `XILINX_PCIE_REG_MSIBASE1` | MSI 目标地址高 32 位 |
| 0x150 | `XILINX_PCIE_REG_MSIBASE2` | MSI 目标地址低 32 位 |
| 0x154 | `XILINX_PCIE_REG_RPEFR` | Root Port 错误 FIFO 读（错误详情） |
| 0x158 | `XILINX_PCIE_REG_RPIFR1` | Root Port 中断 FIFO 读 1（INTx/MSI 判定） |
| 0x15c | `XILINX_PCIE_REG_RPIFR2` | Root Port 中断 FIFO 读 2（MSI 向量号） |

> 这些寄存器是 **IP 的 AXI 侧寄存器**（RC 自己），不是下游 EP 的配置空间。RC 驱动通过它们控制/查询"这座桥"本身。

---

## 1.3 三栏映射表（核心）

| 寄存器（硬件） | 硬件概念（阶段 0） | 软件行为（pcie-xilinx.c） |
|---|---|---|
| `PSCR` bit11 `LNKUP` | 0.1 拓扑/链路：物理链路是否建立 | `xilinx_pcie_link_up()`（:123）读该位；`init_port` 打印 Link UP/DOWN（:501） |
| `RPSC` bit0 `BEN` | 0.1 Bridge 使能：RC 开始转发事务 | `init_port` 置位（:518-521）——先使能桥，PCI Core 才能扫描 |
| `reg_base + ECAM_OFFSET` | 0.2/0.3 配置空间：BDF→内存地址 | `xilinx_pcie_map_bus()`（:177）返回该地址；`pci_generic_config_read`（access.c:80）读它 |
| `IDR` / `IMR` | 0.5 中断：解码/屏蔽中断源 | `intr_handler` 读 IDR 与 IMR 求 status（:382-385）；`init_port` 屏蔽/清除/使能（:506-516） |
| `RPIFR1` bit30 `MSI_INTR` | 0.5 中断类型判定 | `intr_handler` 判 INTx 还是 MSI（:399-407） |
| `RPIFR1` bits[28:27] | 0.5 INTx 向量号（INTA~INTD） | 右移 27 位得 INTx 序号，交 `leg_domain`（:404-406） |
| `RPIFR2` bits[15:0] | 0.5 MSI 向量号 | 读出后交 `msi_domain->parent`（:400-402） |
| `RPEFR` | 错误详情：错误来源 Request ID | `clear_err_interrupts()`（:133）读/清 |
| `MSIBASE1/2` | 0.5 MSI 目标地址：设备往哪写 | `init_irq_domain` 写入 `virt_to_phys(pcie)` 对齐值（:486-487） |

> **三栏映射的读法**：每一行回答三个问题——"硬件里有什么"（寄存器）→"这在 PCIe 里是什么意思"（概念）→"驱动代码怎么用它"（行为）。
---

## 1.4 配置访问回路（寄存器层面）

以 PCI Core 扫描时读 EP 的 Vendor ID 为例，看一次配置读在寄存器/内存层面如何完成：

```
PCI Core: pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &val)
   |
   v
bus->ops->read  = pci_generic_config_read()          [access.c:80]
   |  addr = bus->ops->map_bus(bus, devfn, where)
   v
xilinx_pcie_map_bus()                                [pcie-xilinx.c:177]
   |- bus->sysdata -> pcie（找回 RC 私有数据）
   |- xilinx_pcie_valid_device()：根总线只允许 devfn==0；
   |   下游总线要求链路 LNKUP（读 PSCR）            [pcie-xilinx.c:153]
   - 返回 pcie->reg_base + PCIE_ECAM_OFFSET(bus, devfn, where)
      = reg_base + (bus<<20 | devfn<<12 | where)      [pci-ecam.h:34]
   v
readl(addr)  <- 一条内存读
   v
（底层）RC 识别该地址落在配置窗口 → 构造 CfgRd0 TLP（含 BDF）
   → 经链路路由到 EP → EP 回完成 TLP → RC 把数据放回 readl 返回值
   v
val 返回给 PCI Core：0xFFFFFFFF = 空槽；厂商 ID = 设备存在
```

**关键**：`xilinx_pcie_valid_device()` 是**硬件合法性检查**在软件里的体现——"根总线只挂一个设备"和"链路必须 UP 才能访问下游"，这些约束来自 IP 的物理特性。

---

## 1.5 中断完整回路（寄存器层面）

EP 发中断（INTx 或 MSI）到 CPU 的完整路径，含寄存器动作：

```
[EP] 产生中断
   |  INTx：断言 INTA~INTD（Msg TLP）；MSI：向 MSIBASE 地址写数据（MWr TLP）
   v
[RC 硬件] 收到事务 → 置 IDR 相应位 + 把事件写进 RPIFR1（中断 FIFO）
   v
[RC 触发 AXI 中断线] → xilinx_pcie_intr_handler() 被调用   [pcie-xilinx.c:343]
   |- status = IDR & IMR                      [读 0x138 & 0x13c]
   |- 若 status & (INTX|MSI)：                 [0x130 bit16/17]
   |    读 RPIFR1                             [0x158]
   |    |- RPIFR1.MSI_INTR 置位 → MSI：读 RPIFR2 低16位得向量号
   |    |     domain = pcie->msi_domain->parent   → generic_handle_domain_irq()
   |    - 否则 → INTx：右移27位得 INTA~INTD 序号
   |          domain = pcie->leg_domain           → generic_handle_domain_irq()
   |- 清 RPIFR1（写全1）
   |- 各类错误位打印并清 RPEFR
   - 清 IDR（写 status 回 IDR 清零）
   v
generic_handle_domain_irq(domain, hwirq)
   → irq_domain 路由到 EP 驱动注册的 ISR
```

**与阶段 0.5 的印证**：
- MSI 路径：EP 往 `MSIBASE` 地址写 → RC 收到 → 读 FIFO 得向量号 → `msi_domain` 路由。这就是"中断=内存写"在 RC 侧的实际处理；
- INTx 路径：4 根线共享一条 AXI 中断线 → 靠 RPIFR1 的 2 位字段区分是哪一根。
---

## 1.6 链路管理与 Bridge Enable

### 链路状态：PSCR.LNKUP

```c
// pcie-xilinx.c:123 —— 读 PSCR 的 bit11
static inline bool xilinx_pcie_link_up(struct xilinx_pcie *pcie)
{
	return (pcie_read(pcie, XILINX_PCIE_REG_PSCR) &
		XILINX_PCIE_REG_PSCR_LNKUP) ? 1 : 0;
}
```

- **硬件含义（阶段 0.1）**：物理层链路训练完成 = 与下游 EP 协商成功，链路可以传输；
- **软件用途**：`xilinx_pcie_valid_device()` 在下游总线访问前先检查 LNKUP——链路没 UP 就访问下游会得到垃圾数据，因此直接拒绝（返回 false，`map_bus` 返回 NULL）。

### Bridge Enable：RPSC.BEN

```c
// pcie-xilinx.c:518-521
/* Enable the Bridge enable bit */
pcie_write(pcie, pcie_read(pcie, XILINX_PCIE_REG_RPSC) |
		 XILINX_PCIE_REG_RPSC_BEN,
	   XILINX_PCIE_REG_RPSC);
```

- **硬件含义**：BEN = 让 RC 的桥逻辑开始"转发"配置/内存/IO 事务到链路上；
- **软件用途**：`xilinx_pcie_init_port()` 里置位，**必须早于 `pci_host_probe()` 的扫描**——否则扫描的 CfgRd TLP 根本发不出去。

### 时序依赖（理解 init_port 为什么在 pci_host_probe 之前）

```
probe 流程：
  parse_dt()     映射寄存器、装中断
  init_port()    查链路 + 使能中断 + 置 BEN（桥可转发）
  init_irq_domain()  建 INTx/MSI 域
  pci_host_probe()   扫描（依赖桥已使能、依赖 IRQ 域已就绪）
```

---

## 1.7 MSI 目标地址机制

### 为什么 MSIBASE 写的是 pcie 结构体的物理地址

```c
// pcie-xilinx.c:479-487 —— init_irq_domain 中
phys_addr_t pa = ALIGN_DOWN(virt_to_phys(pcie), SZ_4K);
...
pcie_write(pcie, upper_32_bits(pa), XILINX_PCIE_REG_MSIBASE1);
pcie_write(pcie, lower_32_bits(pa), XILINX_PCIE_REG_MSIBASE2);
```

```c
// pcie-xilinx.c:216 —— compose_msi_msg：告诉 EP 往哪写、写什么
static void xilinx_compose_msi_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct xilinx_pcie *pcie = irq_data_get_irq_chip_data(data);
	phys_addr_t pa = ALIGN_DOWN(virt_to_phys(pcie), SZ_4K);
	msg->address_lo = lower_32_bits(pa);
	msg->address_hi = upper_32_bits(pa);
	msg->data = data->hwirq;   // 向量号
}
```

**机制（印证阶段 0.5 的"中断=内存写"）**：
1. RC 把 pcie 结构体所在的物理页地址写进自己的 `MSIBASE` 寄存器，作为"MSI 识别窗口"；
2. `compose_msi_msg` 把这个**相同地址**配置给 EP 的 MSI capability——保证两边一致；
3. EP 发 MSI = 向该地址写一个 MWr TLP（数据=向量号）；
4. RC 识别写地址落在 MSI 窗口内 → 从 `RPIFR2` 读向量号 → 经 `msi_domain` 路由到 CPU。

> **要点**：MSIBASE 不是"外部设备可访问的内存"，而是 RC 用来**识别 MSI 写事务的窗口地址**。
---

## 1.8 心智模型总结

### 一句话

> **Xilinx AXI PCIe IP = 一座"把 AXI 世界翻译成 PCIe 世界"的桥**：软件通过 `reg_base`（ECAM 内存窗口）读写下游配置空间，通过 `IDR/IMR/RPIFR`（AXI 侧寄存器）处理 PCIe 中断，通过 `PSCR/RPSC` 管理链路与转发使能，通过 `MSIBASE` 定义 MSI 识别窗口。

### 与阶段 0 的完整对照（五层全部落地）

| 阶段 0 概念 | 本 IP 寄存器 | 软件函数 |
|---|---|---|
| 0.1 拓扑（RC/Root Port） | `RPSC.BEN` | `xilinx_pcie_init_port()` |
| 0.2 分层/TLP（配置=CfgRd） | `reg_base` ECAM 窗口 | `xilinx_pcie_map_bus()` |
| 0.3 配置空间（BDF/ECAM） | `PCIE_ECAM_OFFSET` 公式 | `pci_generic_config_read/write` |
| 0.4 BAR/地址译码 | `BIR`（ECAM 大小） | PCI Core `pci_read_bases()` |
| 0.5 中断（INTx/MSI） | `IDR/IMR/RPIFR1/RPIFR2/MSIBASE` | `xilinx_pcie_intr_handler()` |

### 关键时序（boot 时一次成型）

```
platform probe
  ├─ parse_dt: 映射 reg_base + 装中断
  ├─ init_port: 查链路 + 开中断 + BEN（桥就绪）
  ├─ init_irq_domain: INTx域 + MSI域 + 写 MSIBASE
  ├─ bridge->ops/sysdata: 注入统一接口
  └─ pci_host_probe: 扫描 → 资源 → 绑定（此后 EP 可被驱动使用）
```

---

## 1.9 参考

### 本仓库源码（Linux 6.6.99）

| 文件 | 用途 |
|---|---|
| `drivers/pci/controller/pcie-xilinx.c` | Xilinx RC 驱动（本阶段分析对象） |
| `drivers/pci/access.c` | `pci_generic_config_read/write` |
| `drivers/pci/probe.c` | 扫描、`pci_setup_device`、`pci_size` |
| `include/linux/pci-ecam.h` | `PCIE_ECAM_OFFSET` |
| `include/uapi/linux/pci_regs.h` | 配置空间偏移定义 |
| `include/linux/pci.h` | `pci_host_bridge`/`pci_ops` 定义 |

### 网络教程

- Wikipedia《PCI Express》— 拓扑/分层/TLP
- Wikipedia《PCI configuration space》— BDF/ECAM/BAR
- Wikipedia《Message Signaled Interrupts》— MSI 原理
- 内核《How To Write Linux PCI Drivers》— 驱动流程

> **下一步（阶段 2）**：带着阶段 0 硬件心智模型 + 阶段 1 寄存器映射，进入 `drivers/pci/probe.c`（扫描）→ `setup-bus.c`（资源）→ `pci-driver.c`（绑定）三条主线。