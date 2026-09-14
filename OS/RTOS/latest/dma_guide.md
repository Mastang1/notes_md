# DMA：常用 IP、工作模式、总线影响、描述符与 STM32F103 实战

> 目标：从“真实 DMA IP 怎么设计”出发，建立可迁移到 MCU / SoC / Linux 驱动的 DMA 心智模型。  
> 结论先行：DMA 的本质是**另一个总线 Master**。CPU 只负责配置“从哪里、到哪里、多少、何时触发”，之后 DMA 自己发起总线读写；高级 DMA 再把这些配置放进内存中的 **descriptor（描述符）**，DMA 自己取下一项，从而进一步减少 CPU 介入。

说明：没有公开、可信的数据可以给 DMA IP 做“全球使用量排名”。下文选取公开官方资料中最有代表性的几类：Arm PL080/PL081、Arm DMA-330、Synopsys DesignWare DMA、AMD AXI DMA、Intel mSGDMA，以及 STM32F1 的 basic DMA。

## 1. 常见 DMA IP 与应掌握的通用模式

| IP | 总线/定位 | 关键能力 | 学习价值 |
|---|---|---|---|
| **Arm PL080/PL081** | AMBA AHB，8/2 channel | M2M、P2M/M2P、burst、flow control、**Linked List / Scatter-Gather** | 经典“通用 DMA 控制器”模型 |
| **Arm DMA-330** | AMBA AXI | 可编程 DMA 指令流、多 channel/thread | 高级 SoC DMA 模型 |
| **Synopsys DW_ahb_dmac / DW_axi_dmac** | AHB / AXI | 多 channel、多 master interface、SoC 可复用 IP | 商用 SoC 中非常典型的可授权 IP |
| **AMD AXI DMA** | AXI4-MM ↔ AXI4-Stream | Direct Register、SG descriptor、cyclic、2-D/stride | 描述符 DMA 最清晰的公开资料之一 |
| **Intel mSGDMA** | Avalon-MM / ST | descriptor、prefetch、scatter-gather | FPGA/SoC 描述符模型 |
| **STM32F103 DMA** | AHB basic DMA | 7 channel、M2M、P2M/M2P、normal、circular | MCU 最小 DMA 模型；**无硬件 descriptor** |

综合这些 IP，最常见的工作模式可以抽象成下面 7 类：

1. **Single-block / Direct Register**：CPU 写源地址、目的地址、长度，然后启动一次传输。
2. **Peripheral-to-Memory / Memory-to-Peripheral**：外设 request/ack 驱动，例如 UART、SPI、ADC。
3. **Memory-to-Memory**：软件启动，用于 memcpy、搬表、初始化 RAM。
4. **Circular / Ring**：传到尾部自动回到起点，适合连续 ADC、音频、UART 流。
5. **Ping-Pong / Double Buffer**：A/B 两块缓冲区交替，DMA 操作 A 时 CPU 处理 B。
6. **Scatter-Gather / Linked-List Descriptor**：多个不连续 buffer 由 descriptor 链描述，DMA 自动逐项执行。
7. **Cyclic Descriptor / 2-D Interleaved**：descriptor 首尾成环，或带 stride 做图像行/矩阵搬运。

其中 **burst、FIFO、priority、request mux** 更准确地说是“传输属性/性能机制”，不是与 SG、circular 同一级的模式。

---

## 2. 各模式到底怎么工作：场景、流程与差异

### 2.1 Normal / Single-block：UART 发送一帧

场景：`tx_buf[128] -> USART1_DR`。

```text
CPU:
  配 DMA(src=tx_buf, dst=USART_DR, len=128)
  打开 USART TX DMA request
  -> 回去干别的事

USART TXE:
  request DMA
DMA:
  读 tx_buf[n]
  写 USART_DR
  n++
重复 128 次
DMA:
  TC interrupt
CPU:
  处理“这一帧发送完成”
```

特点：
- 外设寄存器地址固定，内存地址递增。
- DMA 并不是“一启动就连续把 128 字节灌进去”，而是由 USART 的 TX empty 请求**节拍化**地搬。
- CPU 从“每字节中断一次”降低为“一帧一次完成中断”。

### 2.2 Peripheral-to-Memory：ADC 连续采样

场景：`ADC_DR -> adc_buf[]`。

每次 ADC EOC 产生 DMA request：

```text
ADC conversion complete
        |
        v
     DMA request
        |
DMA read ADC_DR
        |
DMA write SRAM[n]
        |
     n++
```

Normal 模式采 N 点后停止；Circular 模式采到末尾自动从 `adc_buf[0]` 继续。

适用：
- ADC 波形采集
- UART/SPI RX
- I2S/Audio RX
- 定时器捕获数据

### 2.3 Memory-to-Memory：大块数据复制

CPU 写好：

```text
source = Flash/SRAM A
dest   = SRAM B
length = N
MEM2MEM = 1
EN = 1
```

DMA 无需外设 request，EN 后立即开始：

```text
DMA read source -> internal datapath/FIFO -> DMA write destination
```

典型场景：
- 大块 memcpy
- 图像/网络 buffer 搬运
- Flash 常量表复制到 RAM

差异：它通常是**最容易与 CPU 抢存储器/总线**的 DMA 类型，因为 DMA 会尽可能持续地产生总线事务。

### 2.4 Circular：连续数据流

以 1024 点 ADC buffer 为例：

```text
[0 ........ 511][512 ........ 1023]
      HT IRQ              TC IRQ
         \___________________/
                repeat
```

DMA 到 1024 后：
- 计数器自动恢复为 1024；
- 内存地址恢复到 buffer 起始地址；
- 不需要 CPU 重新配置。

场景：
- ADC 永久采样
- 音频 PCM
- UART 固定长度流
- PWM 波表循环输出

注意：**Circular 不等于 descriptor**。Circular 通常只是“同一块连续 buffer 反复使用”。

### 2.5 Ping-Pong / Double Buffer：实时处理最常用模型

逻辑模型：

```text
时间 --->

DMA : [写 A][写 B][写 A][写 B]...
CPU :      [处理A][处理B][处理A]...
```

高级 DMA 有两个 Memory Address Register，硬件自动 A/B 切换。

**STM32F103 没有硬件 DBM**，但可用：
- 一个 circular buffer；
- Half Transfer 中断代表“前半块好了”；
- Transfer Complete 中断代表“后半块好了”。

所以 F103 可以实现**软件语义上的 ping-pong**，但不是 F4/F7 那种双地址寄存器 DBM。

### 2.6 Scatter-Gather / Linked List：描述符 DMA

这是高级 DMA 必须理解的模式。

CPU 不再每次写 DMA 的 `SRC/DST/LEN` 寄存器，而是在 RAM 中放“DMA 命令”：

```c
struct dma_desc {
    uint32_t src;
    uint32_t dst;
    uint32_t next;
    uint32_t control;   // len / width / burst / irq...
};
```

Arm PL080 的 LLI 就是这个经典模型：一个 LLI 描述一个 block，完成后硬件通过 `next` 自动加载下一项；`next = 0` 表示链结束。

例如网络包：

```text
Descriptor 0 -> Ethernet header buffer
       |
       v
Descriptor 1 -> payload fragment A
       |
       v
Descriptor 2 -> payload fragment B
       |
       v
      NULL
```

CPU 不需要先 memcpy 成连续大包，DMA 直接把多个离散 buffer 拼成一次输出，这就是 **gather**；反过来将输入拆到多个 buffer，就是 **scatter**。

AMD AXI DMA 的 SG 模式也是同一思想：descriptor 中保存 Next Descriptor、Buffer Address、Control/Length、Status 等字段。

### 2.7 Cyclic Descriptor：真正的“descriptor ring”

把最后一个 descriptor 的 next 指回第一个：

```text
 +------ desc0 <------+
 |         |          |
 |         v          |
 |       desc1        |
 |         |          |
 |         v          |
 +------ desc2 -------+
```

DMA 可以永远循环执行，CPU 只在“某个 descriptor 完成”时回收/补充 buffer。

常见场景：
- 网卡 RX/TX ring
- 音频 DMA ring
- 摄像头/视频 frame ring
- 高吞吐串流

它比 basic circular 强在：
- 每个 descriptor 可以是**不同地址**；
- 每个 descriptor 可以是**不同长度**；
- 可以按包设置 status/interrupt/ownership；
- CPU 可以动态回收和补充 descriptor。

---

## 3. 描述符模式要真正理解什么

### 3.1 Descriptor 不是“数据”，而是 DMA 的工作指令

典型字段：

```text
+-------------------+
| next descriptor   |
+-------------------+
| buffer address    |
+-------------------+
| length/control    |
+-------------------+
| status/ownership  |
+-------------------+
```

工作流程：

```text
CPU 建 descriptor
   |
   v
CPU 写 head/current descriptor 寄存器
   |
   v
DMA fetch descriptor --------+
   |                         |
   v                         |
读/写真正的数据 buffer        |
   |                         |
   v                         |
DMA 更新 status              |
   |                         |
   v                         |
读取 next descriptor --------+
```

CPU 只需在链首/链尾、回收 buffer 或错误处理中介入。

### 3.2 为什么网卡、PCIe、SDIO、高速音视频特别喜欢 descriptor

因为这些系统的数据天然是：
- 多 packet；
- 多 buffer；
- 不连续；
- 高速连续到达；
- CPU 不可能每完成一个小块就重写 DMA 寄存器。

descriptor 把“下一次干什么”提前排成队列，使 DMA 的控制平面也部分硬件化。

### 3.3 descriptor 最容易出 bug 的地方

1. **ownership**：CPU 和 DMA 不能同时改同一 descriptor。
2. **cache coherency**：有 D-cache 的 SoC 上，CPU 写 descriptor 后 DMA 未必立即看到；DMA 写 status 后 CPU cache 也可能仍是旧值。
3. **memory barrier**：必须保证“先写 buffer/descriptor，再把 ownership/head 交给 DMA”的顺序。
4. **alignment**：很多 IP 要求 descriptor 32/64-byte 对齐。
5. **ring full / empty**：producer/consumer 指针管理。
6. **中断风暴**：通常不是每个 descriptor 都 IRQ，而是做 interrupt coalescing。

**STM32F103 Cortex-M3 无 D-cache，因此没有 cache maintenance 问题；但它本身也没有 descriptor fetch engine。**

### 3.4 F103 能否“模拟 descriptor”？

可以软件模拟，但本质不同：

```text
DMA block0 complete IRQ
        |
CPU 读取 software_desc[1]
        |
CPU disable DMA
CPU 重写 CPAR/CMAR/CNDTR/CCR
        |
CPU enable DMA
```

这叫**软件 chaining**，不是硬件 SG：
- 每块之间 CPU 必须介入；
- 有 ISR latency；
- 块与块之间可能有 gap；
- 无法达到真正 descriptor DMA 的吞吐和低 CPU 占用。

---

## 4. DMA 什么时候会影响 CPU：必须从总线看

ST AN2548 明确给出 STM32 basic DMA 的核心模型：

```text
CPU --------\
             +---- Bus Matrix ---- SRAM / Flash / AHB slave
DMA --------/          |
                       +---- AHB/APB bridge ---- ADC/UART/SPI...
```

CPU 和 DMA 都是 **bus master**。

### 4.1 不一定互相影响

如果 CPU 和 DMA 同时访问**不同的 bus-matrix slave port**，可以并行。

例：

```text
CPU: Flash 取指
DMA: APB ADC -> SRAM
```

只要路径没有争用同一 slave/bridge，影响可以很小。

### 4.2 会明显影响 CPU 的典型情况

**场景 A：DMA Flash -> SRAM，而 CPU 同时从 Flash 取指**

```text
CPU ----> Flash <---- DMA
```

两者争 Flash slave port，产生 arbitration / wait。

**场景 B：DMA 高频写 SRAM，而 CPU 高频访问同一 SRAM**

```text
CPU ----> SRAM <---- DMA
```

CPU load/store latency 会增大。

**场景 C：多个 DMA channel 同时跑**

DMA 内部还要先仲裁 channel，再作为一个 master 去争外部总线。

**场景 D：AXI DMA 使用很长 burst**

现代 AXI DMA 能提高带宽，但长 burst / QoS 配置不当会提高其他 master 的访问延迟。

### 4.3 F103/basic DMA 的关键点

ST AN2548 描述 basic DMA 时指出：
- DMA 通过独立 AHB master port 访问 bus matrix；
- CPU 与 DMA 访问同一目标时需要 arbitration；
- basic DMA 偏向低 latency sharing，而不是让 DMA 长时间独占总线；
- 单个 DMA 数据项本质是“读源 + 写目的”两次总线访问；
- APB 外设还要经过 AHB-to-APB bridge。

因此面试中说“DMA 完全不占 CPU、完全不影响 CPU”是错的。

正确说法：

> DMA 不需要 CPU 执行每次数据搬运指令，但它会占用互连、存储器和外设总线资源；与 CPU 访问路径发生竞争时，CPU 会看到额外等待时间。

---

## 5. STM32F103：寄存器模型

DMA1 基址：`0x40020000`。DMA1 有 7 个 channel。

每个 channel 的关键寄存器：

| Register | 含义 |
|---|---|
| `DMA_ISR` | GIF/TCIF/HTIF/TEIF 状态 |
| `DMA_IFCR` | 清中断标志 |
| `DMA_CCRx` | 模式/宽度/递增/优先级/中断/使能 |
| `DMA_CNDTRx` | 剩余数据项数，最大 65535 |
| `DMA_CPARx` | “Peripheral side” 地址；M2M 时也可作为普通 memory 地址 |
| `DMA_CMARx` | “Memory side” 地址 |

`DMA_CCRx` 最重要位：

```text
14 MEM2MEM
13:12 PL      00 low, 01 medium, 10 high, 11 very high
11:10 MSIZE   00 8-bit, 01 16-bit, 10 32-bit
 9:8  PSIZE   00 8-bit, 01 16-bit, 10 32-bit
 7    MINC
 6    PINC
 5    CIRC
 4    DIR      0: read CPAR side; 1: read CMAR side
 3    TEIE
 2    HTIE
 1    TCIE
 0    EN
```

F103 DMA1 常用固定映射：

```text
Channel1: ADC1
Channel2: SPI1_RX
Channel3: SPI1_TX
Channel4: USART1_TX
Channel5: USART1_RX
Channel6: USART2_RX
Channel7: USART2_TX
```

同一 channel 上可能 OR 多个 peripheral request，因此**同一时刻不要让映射到同一 channel 的多个外设同时发 DMA request**。

配置顺序必须形成肌肉记忆：

```text
1. EN=0
2. clear flags
3. CPAR
4. CMAR
5. CNDTR
6. CCR: DIR/CIRC/INC/SIZE/PL/IRQ
7. peripheral enable DMA request（P2M/M2P）
8. EN=1
```

`CPAR/CMAR/CNDTR` 均不应在 channel enabled 时重写。

---

## 6. STM32F103 寄存器级 Demo

以下使用 CMSIS `STM32F10x` 寄存器结构，重点只展示 DMA。

### Demo A：Memory-to-Memory，一次性复制 32-bit 数组

```c
#include "stm32f10x.h"

void dma_memcpy32(uint32_t *dst, const uint32_t *src, uint16_t words)
{
    RCC->AHBENR |= (1U << 0);             // DMA1EN

    DMA1_Channel1->CCR &= ~(1U << 0);     // EN=0
    DMA1->IFCR = 0x0FU;                   // clear CH1 GIF/TC/HT/TE

    DMA1_Channel1->CPAR  = (uint32_t)src; // DIR=0: source side
    DMA1_Channel1->CMAR  = (uint32_t)dst; // destination side
    DMA1_Channel1->CNDTR = words;

    DMA1_Channel1->CCR =
          (1U << 14)  // MEM2MEM
        | (2U << 12)  // PL=high
        | (2U << 10)  // MSIZE=32-bit
        | (2U << 8)   // PSIZE=32-bit
        | (1U << 7)   // MINC
        | (1U << 6)   // PINC
        | (1U << 1);  // TCIE
                      // DIR=0, CIRC=0

    DMA1_Channel1->CCR |= (1U << 0);      // start immediately
}
```

这里把 `CPAR` 当 source、`CMAR` 当 destination；MEM2MEM 下它们只是 DMA 两侧地址寄存器，不要求 CPAR 真的是外设。

### Demo B：USART1_TX Normal DMA

USART1_TX 固定映射到 DMA1 Channel4。

```c
void usart1_tx_dma(uint8_t *buf, uint16_t len)
{
    RCC->AHBENR |= (1U << 0);            // DMA1 clock

    DMA1_Channel4->CCR &= ~(1U << 0);
    DMA1->IFCR = (0x0FU << 12);          // clear CH4 flags

    DMA1_Channel4->CPAR  = (uint32_t)&USART1->DR;
    DMA1_Channel4->CMAR  = (uint32_t)buf;
    DMA1_Channel4->CNDTR = len;

    DMA1_Channel4->CCR =
          (1U << 7)   // MINC
        | (1U << 4)   // DIR=1: memory -> CPAR side
        | (1U << 1);  // TCIE
                      // 8-bit, PINC=0, CIRC=0

    USART1->CR3 |= (1U << 7);            // DMAT
    DMA1_Channel4->CCR |= (1U << 0);     // EN
}
```

工作流：

```text
USART TXE -> request CH4
CH4: RAM[n] -> USART1_DR
USART 移出一个字节
再次 TXE -> 下一次 DMA request
...
CNDTR=0 -> TC
```

### Demo C：ADC1 Circular + HT/TC，模拟 Ping-Pong

```c
#define ADC_N 128
volatile uint16_t adc_buf[ADC_N];

void adc_dma_circular_init(void)
{
    RCC->AHBENR |= (1U << 0);

    DMA1_Channel1->CCR &= ~(1U << 0);
    DMA1->IFCR = 0x0F;

    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)adc_buf;
    DMA1_Channel1->CNDTR = ADC_N;

    DMA1_Channel1->CCR =
          (2U << 12)  // high priority
        | (1U << 10)  // MSIZE=16-bit
        | (1U << 8)   // PSIZE=16-bit
        | (1U << 7)   // MINC
        | (1U << 5)   // CIRC
        | (1U << 3)   // TEIE
        | (1U << 2)   // HTIE
        | (1U << 1);  // TCIE
                      // DIR=0: ADC_DR -> RAM

    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    ADC1->CR2 |= (1U << 8);              // ADC DMA enable
    DMA1_Channel1->CCR |= (1U << 0);
}
```

ISR：

```c
void DMA1_Channel1_IRQHandler(void)
{
    if (DMA1->ISR & (1U << 2)) {         // HTIF1
        DMA1->IFCR = (1U << 2);
        process_adc_block(&adc_buf[0], ADC_N / 2);
    }

    if (DMA1->ISR & (1U << 1)) {         // TCIF1
        DMA1->IFCR = (1U << 1);
        process_adc_block(&adc_buf[ADC_N / 2], ADC_N / 2);
    }

    if (DMA1->ISR & (1U << 3)) {         // TEIF1
        DMA1->IFCR = (1U << 3);
        // error handling
    }
}
```

这就是 F103 最实用的“伪双缓冲”：

```text
DMA 正写后半区 -> CPU 处理前半区
DMA 回到前半区 -> CPU 处理后半区
```

验收时用示波器或 GPIO 打点测 `HT IRQ`、`TC IRQ` 和 `process` 时间，要求：

```text
T_process(half buffer) < T_fill(half buffer)
```

否则 CPU 来不及消费数据，会发生逻辑意义上的 overrun。

---

## 7. DMA 高频面试问题：简短答案

**Q1：DMA 为什么能降低 CPU 占用？**  
A：DMA 自己作为 bus master 完成读源/写目的；CPU 只做配置和完成处理，不再逐字节 load/store。

**Q2：DMA 会不会影响 CPU 性能？**  
A：会。它不占 CPU 指令执行，但会占 bus/memory/peripheral bandwidth；CPU 与 DMA 访问同一 slave/bridge 时发生 arbitration 和 wait。

**Q3：DMA 的一次传输实际发生什么？**  
A：至少包括 DMA 仲裁、读 source、写 destination；外设 DMA 还受 request/ack 和 APB/AHB bridge latency 影响。

**Q4：P2M 为什么 peripheral address 通常不递增？**  
A：DMA 始终读同一个外设数据寄存器/FIFO；memory buffer 地址递增。

**Q5：Normal 与 Circular 最大区别？**  
A：Normal 到 NDT=0 后停止服务；Circular 自动恢复计数和初始地址，继续处理 request。

**Q6：Circular 与 descriptor ring 有什么区别？**  
A：Circular 通常重复同一连续 buffer；descriptor ring 可以让每个节点有独立 buffer 地址、长度、控制和状态。

**Q7：Scatter-Gather 解决什么问题？**  
A：把多个非连续 buffer 组成一个 DMA transaction，避免 CPU 先 memcpy 成连续 buffer，并允许 DMA 自动 chaining。

**Q8：什么是 descriptor？**  
A：放在内存中的 DMA 命令，典型包含 next、buffer address、length/control、status；硬件 DMA 自己 fetch。

**Q9：为什么 descriptor DMA 要考虑 cache？**  
A：CPU cache 中的新 descriptor/data 可能尚未写回 RAM，DMA 看不到；DMA 更新的 status 也可能被 CPU cache 遮住，因此要做 cache clean/invalidate 和 memory barrier。

**Q10：STM32F103 支持硬件 descriptor/SG 吗？**  
A：不支持。它是 basic channel DMA；可以 ISR 中重新装载寄存器做 software chaining，但 CPU 必须介入。

**Q11：DMA priority 是 CPU 与 DMA 的优先级吗？**  
A：通常不是。F103 的 PL 主要用于 DMA controller 内部多个 channel 的仲裁；DMA 与 CPU 的竞争由 bus matrix/interconnect arbitration 决定。

**Q12：为什么 DMA 传大块数据时 CPU 反而可能变慢？**  
A：DMA 持续访问 SRAM/Flash/DDR，增加同一目标上的总线竞争、存储器排队和 cache/memory pressure。

**Q13：HT interrupt 有什么工程价值？**  
A：把一个 circular buffer 分成两半，DMA 写一半时 CPU 处理另一半，形成低成本 ping-pong pipeline。

**Q14：什么时候不用 DMA？**  
A：数据极少、设置 DMA 的成本大于 memcpy/中断；或实时路径对 bus jitter 极敏感且 DMA 会制造更大争用时。

---

## 8. 一句话心智模型

```text
最小 DMA:
CPU 写 SRC/DST/LEN -> DMA 搬一次

流式 DMA:
Peripheral request -> DMA 搬一个 beat -> 循环

Circular:
同一 buffer 自动回卷

Ping-Pong:
DMA 和 CPU 并行处理两块 buffer

Descriptor DMA:
CPU 把未来多个 DMA 操作写成内存任务链
DMA 自己 fetch descriptor + 搬数据 + 找 next
```

真正理解 DMA，不要只记 `HAL_DMA_Start()`；要盯住四件事：

**谁产生 request → DMA 读哪里/写哪里 → 走哪条 bus → 下一块工作由 CPU 还是 descriptor 提供。**

---

## 官方资料

1. ST RM0008 — STM32F101/102/103/105/107 Reference Manual  
   https://www.st.com/resource/en/reference_manual/cd00171190.pdf

2. ST AN2548 — Introduction to DMA controller for STM32 MCUs  
   https://www.st.com/resource/en/application_note/an2548-introduction-to-dma-controller-for-stm32-mcus-stmicroelectronics.pdf

3. ST STM32F103 documentation page  
   https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html

4. Arm PrimeCell DMA Controller PL080 TRM, ARM DDI 0196  
   https://documentation-service.arm.com/static/5e8e3c6488295d1e18d3a8c3

5. Arm PL080 Product Support  
   https://support.arm.com/compute-ip/pl080-primecell-dma-controller

6. Arm DMA-330 Technical Reference Manual  
   https://developer.arm.com/documentation/ddi0424/latest/

7. Synopsys DesignWare AHB DMA Controller  
   https://www.synopsys.com/designware-ip/soc-infrastructure-ip/amba/amba-ahb-dma.html

8. Synopsys DW_axi_dmac IP Directory  
   https://www.synopsys.com/dw/ipdir.php?c=DW_axi_dmac

9. AMD AXI DMA Product Guide PG021 — Feature / Direct / Scatter-Gather / Cyclic modes  
   https://docs.amd.com/r/en-US/pg021_axi_dma/Feature-Summary  
   https://docs.amd.com/r/en-US/pg021_axi_dma/Direct-Register-Mode-Simple-DMA  
   https://docs.amd.com/r/en-US/pg021_axi_dma/Scatter/Gather-Mode  
   https://docs.amd.com/r/en-US/pg021_axi_dma/Cyclic-DMA-Mode

10. Intel Modular Scatter-Gather DMA Core  
    https://www.intel.com/content/www/us/en/docs/programmable/683130/current/modular-scatter-gather-dma-core.html

11. Linux kernel DMAengine controller/client documentation（用于验证通用 DMA_MEMCPY / DMA_SLAVE / DMA_CYCLIC / DMA_INTERLEAVE / SG 抽象）  
    https://docs.kernel.org/driver-api/dmaengine/provider.html  
    https://docs.kernel.org/driver-api/dmaengine/client.html
