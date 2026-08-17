---
tags:
  - OS
  - Linux
---

这份**原始 `ranges` + 原始报错日志**把问题基本定死了。之前“只是缺少 Prefetchable range”还不够精确，真正的问题是：

> **原始 DTS 用 `0x82000000` 声明了一个“32-bit PCI Memory Space”，却给了它接近 32GB 的 PCI 子地址范围；Hailo-8 的 64-bit Prefetchable BAR 最终被 Linux 分配到了 PCI Bus Address ≥ 4GB 的位置，超出了这个 32-bit PCI Memory Space 的语义范围。随后 BAR 虽然“分配成功”，但实际 MMIO 访问失败，Hailo probe 最终报 `Failed init pcie resources`。**

### 证据 1：原始 `ranges` 本身存在 **32/64-bit 属性矛盾**

原始：

```dts
<0x82000000 0x0 0x00000000
 0x48       0x00000000
 0x7        0xfffe0000>;
```

其中 `0x82000000` 明确定义的是：

```text
PCI Memory
Non-Prefetchable
32-bit address space
```

Open Firmware PCI binding 明确规定 `ss=10` 是 **32-bit-address Memory Space**，`ss=11` 才是 64-bit Memory Space。 NXP 自己的 PCIe 地址转换教程也明确把 `0x82000000` 解释为“Non-prefetchable + 32-bit address space”。([NXP Community](https://community.nxp.com/t5/i-MX-Processors-Knowledge-Base/Demystifying-the-PCIe-and-CPU-address-space-translation-in-Linux/ta-p/2198806 "Demystifying the PCIe and CPU address space translation in Linux - NXP Community"))

但你却给它：

```text
PCI child base = 0
size ≈ 0x7_FFFE0000 ≈ 32GB
```

也就是说它实际上描述到了：

```text
PCI 0x00000000
~
PCI 0x7_FFFDFFFF
```

明显超过了 32-bit PCI Memory Space 的：

```text
0x00000000 ~ 0xFFFFFFFF
```

**这才是原 DTS 最核心的错误。**

---

### 证据 2：你的日志正好证明 Hailo BAR 被分到了 4GB 边界以上

原始 range 的转换关系是：

```text
CPU 0x48_00000000
        ↓
PCI 0x00000000
```

所以 offset：

```text
CPU - PCI = 0x48_00000000
```

而日志显示 Hailo：

```text
BAR0 → CPU 0x49_00000000
BAR4 → CPU 0x49_00004000
BAR2 → CPU 0x49_00008000
```

转换成真正写进 EP BAR 的 **PCI Bus Address**：

```text
BAR0:
0x49_00000000 - 0x48_00000000
= 0x1_00000000

BAR4:
= 0x1_00004000

BAR2:
= 0x1_00008000
```

注意：

```text
0x1_00000000 = 4GB
```

也就是说：

> **Hailo 的三个 BAR 恰好全部被放到了 32-bit 地址空间上限之外。**

这和原来的 `0x82000000 = 32-bit PCI Memory Space` 直接矛盾。

---

### 证据 3：NXP/DWC 驱动确实这样做地址转换

你给的 NXP Linux 里，DesignWare Host 代码：

```c
dw_pcie_prog_outbound_atu(
    pci,
    i,
    PCIE_ATU_TYPE_MEM,
    entry->res->start,
    entry->res->start - entry->offset,
    resource_size(entry->res));
```

也就是说：

```text
CPU address
     ↓
减掉 range offset
     ↓
PCI Bus Address
```

就是上面算出来的：

```text
CPU 0x49_00000000
→ PCI 0x1_00000000
```

不是猜测，是 NXP 使用的 DesignWare Host 源码行为。 NXP 官方自己的 PCIe 地址转换说明也是同样的计算模型，并明确指出 `ranges` 最终通过 `dw_pcie_iatu_setup()` 编程 iATU。([NXP Community](https://community.nxp.com/t5/i-MX-Processors-Knowledge-Base/Demystifying-the-PCIe-and-CPU-address-space-translation-in-Linux/ta-p/2198806 "Demystifying the PCIe and CPU address space translation in Linux - NXP Community"))

---

## 证据 4：为什么日志看起来 BAR 明明“assigned”成功，却 Hailo 失败？

你的日志：

```text
pci 0000:01:00.0: BAR 0: assigned [...]
pci 0000:01:00.0: BAR 4: assigned [...]
pci 0000:01:00.0: BAR 2: assigned [...]
...
hailo 0000:01:00.0: Probing: Failed init pcie resources
```

这非常关键：

**Linux resource allocator 成功 ≠ 实际 MMIO 访问成功。**

Hailo driver 的流程是：

```text
pci_enable_device
↓
pci_request_regions
↓
pci_iomap(BAR0)
pci_iomap(BAR2)
pci_iomap(BAR4)
↓
真正读取 Hailo BAR0 内容
```

源码明确：三个 BAR 映射完成后，驱动执行：

```c
hailo_pcie_is_device_connected(resources)
```

它实际上读取：

```text
BAR0 + 0x98
```

并检查是不是 Hailo Vendor ID。

如果真实 BAR MMIO 访问不到设备，就报：

```text
Probing: Failed reading device BARs, device may be disconnected
```

随后外层正好就是你看到的：

```text
Probing: Failed init pcie resources
```

而且 Hailo 官方论坛里 **NXP i.MX8MP + Hailo-8** 有完全一样的现象：

```text
lspci 能发现 1e60:2864
BAR0/2/4 都 mapped
↓
Failed reading device BARs
↓
Failed init pcie resources
```

([Hailo Community](https://community.hailo.ai/t/failed-reading-device-bars/2051?utm_source=chatgpt.com "Failed reading device BARs - General Discussion"))

---

# 为什么你修改后的 DTS 能解决？

修改后：

```dts
<0x82000000 0x0 0x00000000
 0x48       0x00000000
 0x0        0x40000000>

<0x43000000 0x49 0x00000000
 0x49       0x00000000
 0x0        0x80000000>
```

现在变成：

```text
32-bit Non-prefetchable
PCI 0 ~ 0x3FFFFFFF
→ 只有1GB
→ 合法

64-bit Prefetchable
PCI 0x49_00000000 ~ ...
→ CPU 0x49_00000000
→ 合法
```

而 Hailo：

```text
BAR0/2/4
= 64-bit Prefetchable
```

自然进入：

```text
0x43000000
= 64-bit Prefetchable PCI Memory Space
```

并且这次：

```text
CPU 0x49_00000000
        ↓ 1:1
PCI 0x49_00000000
```

地址类型、BAR属性、Host Window 全部一致。

---

## 当前最终结论

现在我会把这个 Bug 定义为：

> **PCIe1 Device Tree `ranges` 地址空间属性定义错误/不完整。原配置使用 `0x82000000`（32-bit Non-Prefetchable PCI Memory Space）却覆盖了约 32GB 的 PCI child address range，导致 Linux 将 Hailo-8 的 64-bit Prefetchable BAR 分配到 PCI Bus Address `0x1_00000000` 以上。资源管理层虽然完成 BAR assignment，但实际 RC → EP BAR MMIO 路径失败，Hailo 在首次 BAR sanity read 时 probe 失败。**

所以真正的修复不是简单：

> “Hailo 要求必须有 `0x43`。”

而是：

> **必须让 Host Bridge 的 `ranges` 正确描述 RC 的 32-bit NP 和 64-bit Prefetchable 地址能力。你的修正版正好做到了这一点。**

最后只差一个命令可以把 Hailo 失败点也100%打印出来。原图用了：

```bash
dmesg | grep pci
```

把很多 Hailo 日志过滤掉了。原配置下执行：

```bash
dmesg | grep -i hailo
```

我预期你会直接看到：

```text
mapped bar 0
mapped bar 2
mapped bar 4
Failed reading device BARs, device may be disconnected
Failed init pcie resources
```

如果是这样，这个 Bug 的因果链就完全闭环了。