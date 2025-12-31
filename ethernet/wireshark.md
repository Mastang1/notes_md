Wireshark 调试以太网帧，本质是在做一件很理工科的事：把**线上飞过来的比特**拆成**你能理解的结构**，然后判断“它到底该不该这样飞”。我们从工程视角，一步步来，不玄学。

---
### 捕获过滤器（抓包时生效，BPF）

在开始抓包前设置，效率高：

```text
ether src 00:11:22:33:44:55
ether dst ff:ff:ff:ff:ff:ff
ether proto 0x88cc      # LLDP
vlan                    # 所有 VLAN 帧
```

### 显示过滤器（抓完再筛）

这是 Wireshark 的灵魂：

```text
eth.src == 00:11:22:33:44:55
eth.dst == ff:ff:ff:ff:ff:ff
eth.type == 0x0800      # IPv4
eth.type == 0x0806      # ARP
eth.type == 0x88F7      # 自定义 EtherType
```

如果你在写**自定义以太网协议**，先记住：

> **eth.type 是你世界的“国籍代码”**

---

第三步：真正读懂以太网帧结构。

最原始的以太网帧长这样（不含前导码）：

```
| Dest MAC | Src MAC | Type/Length | Payload | FCS |
|  6B      |   6B    |     2B      | 46~1500 | 4B |
```

在 Wireshark 里展开：

```
Frame
└── Ethernet II
    ├── Destination: xx:xx:xx:xx:xx:xx
    ├── Source: xx:xx:xx:xx:xx:xx
    ├── Type: 0x0800 (IPv4)
```

关键判断点：

* **Ethernet II vs 802.3**

  * Type ≥ 0x0600 → Ethernet II
  * Type ≤ 1500 → 802.3 Length 字段
* **是否有 VLAN Tag**

  * eth.type == 0x8100（802.1Q）
  * 实际 EtherType 在后一个字段

---

第四步：调试“帧对不对”，工程上看什么？

这是嵌入式/驱动/协议调试的常用 checklist。

### 1️⃣ MAC 地址是否合理

* 源 MAC 是否是你设备的
* 目的 MAC 是否匹配（单播 / 广播 / 组播）
* 广播：`ff:ff:ff:ff:ff:ff`

### 2️⃣ EtherType 是否正确

* ARP：0x0806
* IPv4：0x0800
* IPv6：0x86DD
* 自定义协议：>= 0x88B5（避免冲突）

### 3️⃣ Payload 长度

* 最小 46 字节（不足会 padding）
* 你协议定义 32B，但 Wireshark 显示 46B？
  → **padding，不是 bug**

### 4️⃣ FCS / CRC 错误

* Wireshark 默认**看不到 FCS**
* 需要网卡支持 + 勾选：

  ```
  Edit → Preferences → Protocols → Ethernet
  ☑ Assume packets have FCS
  ```
* CRC 错误通常是：

  * DMA bug
  * endian 错
  * 长度字段算错
