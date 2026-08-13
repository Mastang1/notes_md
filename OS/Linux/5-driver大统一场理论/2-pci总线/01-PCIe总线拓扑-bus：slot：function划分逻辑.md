## PCIe 拓扑简短笔记

**一句话：**

> PCIe 物理上是**点对点 Link**，逻辑上通过 **Bridge/Port 把一条条 Bus 串成树形拓扑**。

典型结构：

```text
Root Complex
    │
Root Bus 00
    │
Root Port 00:01.0
    │
Bus 01
    │
Switch Upstream Port 01:00.0
    │
Bus 02
   /   \
DSP1    DSP2
02:01.0 02:02.0
  │       │
Bus03   Bus04
  │       │
EP1     EP2
03:00.0 04:00.0
```

核心关系：

- **Root Port**：属于上一级 Bus，同时连接下一条 Bus。
    
- **Switch Upstream Port**：连接上游 Bus。
    
- **Switch Downstream Port**：每个 DSP 都是一个 Bridge，各自产生自己的下级 Bus。
    
- **Endpoint**：挂在某条 Bus 上，通常是叶子节点。
    
- **BDF**：`Bus:Device.Function`，表示某个 Function 在这棵 PCIe 树中的逻辑位置。
    
- 普通点对点 Link 下的 EP 通常是 `Dev 0`，如 `03:00.0`。
    

最重要的心智模型：

```text
Bus
 └─ Bridge/Port
      └─ 新 Bus
           └─ Device / Bridge
                └─ ...
```

也就是：**PCIe 的“分级”本质是 Bridge 创建下一级 Bus。**