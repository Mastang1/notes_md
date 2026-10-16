前面没有直接回答你的技术问题。下面给出基于真实硬件和 Linux 源码的实现。

选用 Intel I210 PCIe 千兆网卡，硬件手册为 Intel Document 333016-011，Linux 驱动为 `drivers/net/ethernet/intel/igb/igb_main.c`。

![](https://www.google.com/s2/favicons?domain=https://cdrdv2-public.intel.com&sz=32)

Intel CDRD

+2

## 1. DMA 接收的真实硬件流程

接收路径：Ethernet → I210 DMA → PCIe → Host DDR → MSI-X → ISR → NAPI

这里必须区分三个动作：

1. 驱动预先分配 DDR Buffer、建立 DMA 映射并填写 RX Descriptor。DMA Engine 根据描述符中的地址决定数据写到哪里。
    
2. DMA Engine 自主搬运数据。I210 通过 PCIe 向主机内存写入报文，然后更新描述符完成标志 `DD`。CPU 不负责逐字节搬运。
    
3. CPU 收到中断后执行底半部。ISR 只安排 NAPI Poll，真正的收包、数据处理、缓冲区回收由 Poll 完成。
    

I210 手册 §7.1.4.5 和 §8.10 规定了描述符 Ring、Head/Tail 以及硬件所有权机制。特别注意：不能仅凭 RDH 判断 DMA 是否完成，应以内存中描述符的 DD 状态为依据。

![](https://www.google.com/s2/favicons?domain=https://cdrdv2-public.intel.com&sz=32)

Intel CDRD

+1

## 2. I210 DMA IP：寄存器、描述符和缓冲区

I210 每个 RX Descriptor 为 16 Bytes。驱动使用以下寄存器管理接收队列：

|寄存器|Queue 0 地址|功能|
|---|---|---|
|`RDBAL/RDBAH`|`0xC000/0xC004`|Descriptor Ring 的 64 位 DMA 地址|
|`RDLEN`|`0xC008`|Descriptor Ring 总字节数|
|`RDH`|`0xC010`|硬件维护的接收 Head|
|`RDT`|`0xC018`|驱动提交可用 Descriptor 的 Tail|
|`RXDCTL`|`0xC028`|队列使能、预取和写回阈值|
|`SRRCTL`|`0xC00C`|描述符类型、RX Buffer 大小|

地址及寄存器定义来自 Intel 手册 §8.10。Ring 要求 128 Bytes 对齐，`RDLEN` 必须为 128 的整数倍。

![](https://www.google.com/s2/favicons?domain=https://cdrdv2-public.intel.com&sz=32)

Intel CDRD

+1

一个 RX Descriptor 的本质是：

```
Host DDR
┌──────────────────────────────────────┐
│ RX Descriptor Ring（DMA coherent）   │
│ Desc[0] → Buffer[0] DMA Address      │
│ Desc[1] → Buffer[1] DMA Address      │
│ Desc[2] → Buffer[2] DMA Address      │
└──────────────────────────────────────┘
             │
             ▼
┌──────────────────────────────────────┐
│ RX Buffer（DMA streaming mapping）   │
│ Buffer[0]: 接收到的 Ethernet Frame   │
│ Buffer[1]: 接收到的 Ethernet Frame   │
└──────────────────────────────────────┘
```

这里选用 `igb` 实际采用的 Advanced One-Buffer Descriptor 模式：设备接收完成后写回 `length`、`status_error` 等字段。官方 Linux `igb_clean_rx_irq()` 使用非零 `length` 判断是否有已完成的描述符，随后通过 `dma_rmb()` 确保读取其他写回字段的顺序。

![](https://www.google.com/s2/favicons?domain=https://code.googlesource.com&sz=32)

Git at Google

+1

## 3. C Demo：实际驱动中的 DMA → ISR → NAPI 核心

下面是从 Linux `igb` 驱动精简出来的代码。使用源码已有的结构体、宏和辅助函数，省略了 PCI 枚举、MAC/PHY 初始化、TX、XDP 等不相关部分。它是可对照真实驱动阅读和改造的核心片段，不是独立可加载的完整 PCIe 驱动。

第一部分：初始化 DMA Ring，向硬件提交 RX Buffer

```

/* 依赖 igb_main.c、igb.h 中的类型和硬件宏 */

/* Descriptor Ring 在真实驱动中这样分配：
 * ring->desc = dma_alloc_coherent(ring->dev, ring->size,
 *                                 &ring->dma, GFP_KERNEL);
 */

static void demo_setup_rx(struct igb_adapter *adapter,
                          struct igb_ring *ring)
{
    struct e1000_hw *hw = &adapter->hw;
    int q = ring->reg_idx;
    u64 base = ring->dma;
    u32 ctl;

    /* 1. 配置前关闭 RX 队列 */
    wr32(E1000_RXDCTL(q), 0);

    /* 2. 告知 DMA Engine 描述符 Ring 的 DMA 地址 */
    wr32(E1000_RDBAL(q), lower_32_bits(base));
    wr32(E1000_RDBAH(q), upper_32_bits(base));
    wr32(E1000_RDLEN(q),
         ring->count * sizeof(union e1000_adv_rx_desc));

    wr32(E1000_RDH(q), 0);
    wr32(E1000_RDT(q), 0);

    /* 3. 配置 Advanced One-Buffer Descriptor */
    igb_setup_srrctl(adapter, ring);

    /* 4. 启用描述符获取/写回单元 */
    ctl = IGB_RX_PTHRESH |
          (IGB_RX_HTHRESH << 8) |
          (IGB_RX_WTHRESH << 16) |
          E1000_RXDCTL_QUEUE_ENABLE;

    wr32(E1000_RXDCTL(q), ctl);

    /* 5. 在实际驱动完成接收单元初始化后：
     * 分配/映射 RX Pages，填充 Descriptor，
     * 最后通过 RDT 将缓冲区交给硬件。
     */
    igb_alloc_rx_buffers(ring, ring->count - 1);
}
```

关键不是 CPU 将数据复制到 Buffer，而是 `igb_alloc_rx_buffers()` 完成三个动作：

```
/* 实际驱动核心语句，其他代码省略 */

/* 建立设备可以使用的 DMA 映射 */
dma = dma_map_page_attrs(dev, page, 0, page_size,
                         DMA_FROM_DEVICE, attrs);

/* 设置描述符中的 DMA 目标地址 */
rx_desc->read.pkt_addr = cpu_to_le64(dma + offset);

/* 保证描述符写入完成后再通知硬件 */
dma_wmb();
writel(next_to_use, rx_ring->tail);
```

随后 DMA Engine 从 Ring 读取 Descriptor，获得 Buffer 的 DMA Address，经 PCIe 写入报文数据。

![](https://www.google.com/s2/favicons?domain=https://code.googlesource.com&sz=32)

Git at Google

+2

第二部分：硬中断只调度底半部，NAPI 负责实际收包

```

/* 精简自 igb_msix_ring() */
static irqreturn_t demo_msix_irq(int irq, void *data)
{
    struct igb_q_vector *q = data;

    /* 不在 ISR 中遍历 RX Descriptor */
    napi_schedule(&q->napi);

    return IRQ_HANDLED;
}

/* 精简自 igb_poll()：只保留 RX 路径 */
static int demo_rx_poll(struct napi_struct *napi, int budget)
{
    struct igb_q_vector *q =
        container_of(napi, struct igb_q_vector, napi);

    /* 清理 DMA 完成的描述符，批量处理报文 */
    int done = igb_clean_rx_irq(q, budget);

    if (done < budget) {
        if (napi_complete_done(napi, done))
            igb_ring_irq_enable(q);
    }

    return done;
}
```

实际的 `igb_clean_rx_irq()` 中，最值得关注的是下面这条执行链：

```

/* 1. 读取 DMA 写回的完成信息 */
rx_desc = IGB_RX_DESC(rx_ring, rx_ring->next_to_clean);
size = le16_to_cpu(rx_desc->wb.upper.length);

if (!size)
    break;                 /* 还没完成 */

/* 2. 确保后续读取不会越过完成状态检查 */
dma_rmb();

/* 3. 获取接收 Buffer，并同步 DMA 数据供 CPU 使用 */
rx_buffer = igb_get_rx_buffer(rx_ring, size, &pgcnt);
/* 内部调用 dma_sync_single_range_for_cpu() */

/* 4. 构建 skb，处理多 Descriptor 报文、错误等 */
// igb_build_skb() / igb_construct_skb() ...

/* 5. 将完整报文送入 Linux 网络栈 */
napi_gro_receive(&q_vector->napi, skb);

/* 6. 回收缓冲区后批量补充 Descriptor */
// igb_alloc_rx_buffers(rx_ring, cleaned_count);
```

上述第 4、6 步是展示真实调用位置的摘要，并非可单独编译的连续函数体。真实 `igb` 驱动同时处理多描述符报文、XDP、时间戳、VLAN 和页复用；补充 Buffer 时使用 `dma_sync_single_range_for_device()`，再调用 `dma_wmb()` 和 `writel(RDT)`。

![](https://www.google.com/s2/favicons?domain=https://code.googlesource.com&sz=32)

Git at Google

+2

## 4. UML 执行序列图：完整接收时序

Linux Network StackNAPI Poll / SoftIRQCPU ISRI210 RX DMAHost DDRLinux igb DriverLinux Network StackNAPI Poll / SoftIRQCPU ISRI210 RX DMAHost DDRLinux igb Driver#chatgpt-mermaid-_r_14p_{font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";font-size:16px;fill:rgb(237, 237, 237);}@keyframes edge-animation-frame{from{stroke-dashoffset:0;}}@keyframes dash{to{stroke-dashoffset:0;}}#chatgpt-mermaid-_r_14p_ .edge-animation-slow{stroke-dasharray:9,5!important;stroke-dashoffset:900;animation:dash 50s linear infinite;stroke-linecap:round;}#chatgpt-mermaid-_r_14p_ .edge-animation-fast{stroke-dasharray:9,5!important;stroke-dashoffset:900;animation:dash 20s linear infinite;stroke-linecap:round;}#chatgpt-mermaid-_r_14p_ .error-icon{fill:rgb(48, 48, 48);}#chatgpt-mermaid-_r_14p_ .error-text{fill:rgb(237, 237, 237);stroke:rgb(237, 237, 237);}#chatgpt-mermaid-_r_14p_ .edge-thickness-normal{stroke-width:1px;}#chatgpt-mermaid-_r_14p_ .edge-thickness-thick{stroke-width:3.5px;}#chatgpt-mermaid-_r_14p_ .edge-pattern-solid{stroke-dasharray:0;}#chatgpt-mermaid-_r_14p_ .edge-thickness-invisible{stroke-width:0;fill:none;}#chatgpt-mermaid-_r_14p_ .edge-pattern-dashed{stroke-dasharray:3;}#chatgpt-mermaid-_r_14p_ .edge-pattern-dotted{stroke-dasharray:2;}#chatgpt-mermaid-_r_14p_ .marker{fill:rgb(175, 175, 175);stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ .marker.cross{stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ svg{font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";font-size:16px;}#chatgpt-mermaid-_r_14p_ p{margin:0;}#chatgpt-mermaid-_r_14p_ .actor{stroke:rgb(31, 78, 148);fill:rgb(9, 23, 44);stroke-width:1;}#chatgpt-mermaid-_r_14p_ rect.actor.outer-path[data-look="neo"]{filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ rect.note[data-look="neo"]{stroke:rgb(58, 132, 63);fill:rgb(48, 48, 48);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ text.actor>tspan{fill:rgb(237, 237, 237);stroke:none;}#chatgpt-mermaid-_r_14p_ .actor-line{stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ .innerArc{stroke-width:1.5;stroke-dasharray:none;}#chatgpt-mermaid-_r_14p_ .messageLine0{stroke-width:1.5;stroke-dasharray:none;stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ .messageLine1{stroke-width:1.5;stroke-dasharray:2,2;stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ [id$="-arrowhead"] path{fill:rgb(175, 175, 175);stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ .sequenceNumber{fill:#505050;}#chatgpt-mermaid-_r_14p_ [id$="-sequencenumber"]{fill:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ [id$="-crosshead"] path{fill:rgb(175, 175, 175);stroke:rgb(175, 175, 175);}#chatgpt-mermaid-_r_14p_ .messageText{fill:rgb(237, 237, 237);stroke:none;}#chatgpt-mermaid-_r_14p_ .labelBox{stroke:rgba(255, 255, 255, 0.15);fill:rgb(0, 0, 0);filter:none;}#chatgpt-mermaid-_r_14p_ .labelText,#chatgpt-mermaid-_r_14p_ .labelText>tspan{fill:rgb(237, 237, 237);stroke:none;}#chatgpt-mermaid-_r_14p_ .loopText,#chatgpt-mermaid-_r_14p_ .loopText>tspan{fill:rgb(237, 237, 237);stroke:none;}#chatgpt-mermaid-_r_14p_ .sectionTitle,#chatgpt-mermaid-_r_14p_ .sectionTitle>tspan{fill:rgb(237, 237, 237);stroke:none;}#chatgpt-mermaid-_r_14p_ .loopLine{stroke-width:2px;stroke-dasharray:2,2;stroke:rgba(255, 255, 255, 0.15);fill:rgba(255, 255, 255, 0.15);}#chatgpt-mermaid-_r_14p_ .note{stroke:rgb(58, 132, 63);fill:rgb(48, 48, 48);}#chatgpt-mermaid-_r_14p_ .noteText,#chatgpt-mermaid-_r_14p_ .noteText>tspan{fill:rgb(237, 237, 237);stroke:none;font-weight:normal;}#chatgpt-mermaid-_r_14p_ .activation0{fill:rgb(48, 48, 48);stroke:hsl(0, 0%, 8.8235294118%);}#chatgpt-mermaid-_r_14p_ .activation1{fill:rgb(48, 48, 48);stroke:hsl(0, 0%, 8.8235294118%);}#chatgpt-mermaid-_r_14p_ .activation2{fill:rgb(48, 48, 48);stroke:hsl(0, 0%, 8.8235294118%);}#chatgpt-mermaid-_r_14p_ .actorPopupMenu{position:absolute;}#chatgpt-mermaid-_r_14p_ .actorPopupMenuPanel{position:absolute;fill:rgb(9, 23, 44);box-shadow:0px 8px 16px 0px rgba(0,0,0,0.2);filter:drop-shadow(3px 5px 2px rgb(0 0 0 / 0.4));}#chatgpt-mermaid-_r_14p_ .actor-man circle,#chatgpt-mermaid-_r_14p_ line{fill:rgb(9, 23, 44);stroke-width:2px;}#chatgpt-mermaid-_r_14p_ g rect.rect{filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));stroke:rgb(31, 78, 148);}#chatgpt-mermaid-_r_14p_ .node .neo-node{stroke:rgb(31, 78, 148);}#chatgpt-mermaid-_r_14p_ [data-look="neo"].node rect,#chatgpt-mermaid-_r_14p_ [data-look="neo"].cluster rect,#chatgpt-mermaid-_r_14p_ [data-look="neo"].node polygon{stroke:url(#chatgpt-mermaid-_r_14p_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ [data-look="neo"].swimlane.cluster rect{filter:none;}#chatgpt-mermaid-_r_14p_ [data-look="neo"].node path{stroke:url(#chatgpt-mermaid-_r_14p_-gradient);stroke-width:1px;}#chatgpt-mermaid-_r_14p_ [data-look="neo"].node .outer-path{filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ [data-look="neo"].node .neo-line path{stroke:rgb(31, 78, 148);filter:none;}#chatgpt-mermaid-_r_14p_ [data-look="neo"].node circle{stroke:url(#chatgpt-mermaid-_r_14p_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ [data-look="neo"].node circle .state-start{fill:#000000;}#chatgpt-mermaid-_r_14p_ [data-look="neo"].icon-shape .icon{fill:url(#chatgpt-mermaid-_r_14p_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ [data-look="neo"].icon-shape .icon-neo path{stroke:url(#chatgpt-mermaid-_r_14p_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-_r_14p_ :root{--mermaid-font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";}初始化阶段可以开始读取 RX Descriptor硬件自主接收阶段，CPU 不搬运报文MAC 收到 Ethernet Frame中断上半部ISR 结束，返回中断底半部保持 NAPI 调度，继续批量处理alt[本轮处理量小于 budget，且完成轮询][达到 budget]dma_alloc_coherent() 分配 Descriptor Ring1dma_map_page() 映射 RX Buffer2填入各 Descriptor 的 Buffer DMA Address3MMIO 配置 RDBAL/RDBAH/RDLEN4配置 RXDCTL、SRRCTL5dma_wmb() + 更新 RDT6PCIe Memory Read 获取 RX Descriptor7PCIe Memory Write 报文到 RX Buffer8Descriptor Write-back (Length / Status)9MSI-X 接收中断10napi_schedule()11读取 Descriptor 的完成信息12dma_rmb()13dma_sync_single_range_for_cpu()14有效 RX Buffer 数据15构建 skb + napi_gro_receive()16补充 / 复用 Buffer17dma_wmb() + writel(RDT)18重新使能队列中断19

这里的 ISR 与 NAPI 交接还有一个重要细节：接收中断应在 NAPI Poll 期间保持屏蔽，直至轮询完成，再解除屏蔽。`igb` 的 MSI-X 路径配合 I210 扩展中断的自动屏蔽机制工作，因此 ISR 中不必每次显式写 `EIMC`。

![](https://www.google.com/s2/favicons?domain=https://kernel.googlesource.com&sz=32)

Git at Google

+2

## 5. 这套流程的几个关键点

- DMA Controller 不一定是独立 IP。 I210 的 RX DMA Engine 就集成在网卡中；PCIe 负责承载其 DMA Memory Read/Write 事务。
    
- Descriptor Ring 与 Data Buffer 是两个不同对象。 前者告诉硬件数据写到哪里，后者是真正的数据存储空间。Descriptor 通常使用 coherent DMA，RX Buffer 通常使用 streaming DMA。
    
- 中断不等于一个报文完成。 一个中断可能对应多个已经完成的 RX Descriptor；中断还可能经过 moderation 合并。
    
- 需要两类同步。 `dma_rmb()/dma_wmb()` 主要保证 DMA 相关内存访问顺序；`dma_sync_*()` 负责 streaming DMA 映射在 CPU 与设备之间的可见性和所有权切换。不能简单地互相替代。
    
- NAPI 不等同于普通 workqueue。 Linux 网络驱动的默认 NAPI Poll 通常运行于 SoftIRQ 上下文，也支持 threaded NAPI 模式。
    
    ![](https://www.google.com/s2/favicons?domain=https://cdn.kernel.org&sz=32)
    
    The Linux Kernel documentation
    
    +1
    

## 官方手册和源码入口

1. [Intel I210 Datasheet v3.7](https://cdrdv2-public.intel.com/333016/333016%20-%20I210_Datasheet_v_3_7.pdf?utm_source=chatgpt.com)：重点阅读 §7.1.4（Receive Descriptors）、§7.3（Interrupts）、§8.10（RX Registers）。
    
2. [Linux igb_main.c](https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/intel/igb/igb_main.c?utm_source=chatgpt.com)：按 `igb_configure_rx_ring()` → `igb_alloc_rx_buffers()` → `igb_msix_ring()` → `igb_poll()` → `igb_clean_rx_irq()` 的顺序阅读。
    
3. [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html?utm_source=chatgpt.com)：重点核对 coherent、streaming、DMA 映射方向和 cache 同步要求。
    

最终心智模型： CPU 负责准备描述符和回收资源；DMA Engine 负责 PCIe 数据传输；Descriptor Write-back 负责传递完成状态；ISR 负责触发处理；NAPI 负责批量消费数据并归还 Buffer。这个模型可以迁移到多数基于 Descriptor Ring 的 PCIe NIC、FPGA DMA 和高速通信控制器。