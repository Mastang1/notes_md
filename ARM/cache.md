# 现代高性能 MCU (如 Cortex-M7) Cache 与 Cache 一致性深度技术笔记

---

## 一、 Cache 基础架构与核心工作机制

### 1. 核心概念与术语
* **Linefill（行填充）**：当 CPU 访问内存发生 **Cache Miss（未命中）** 时，总线接口单元（Bus Interface Unit）或 Cache 控制器会发起一次突发传输（Burst Transfer），将主存中以 **Cache Line（缓存行，现代高性能 MCU 通常为 32 字节）** 为单位的对齐数据块批量加载到 Cache 阵列中。
* **局部性原理**：
  * **时间局部性**：当前被访问的数据在不久的将来极大概率再次被访问。
  * **空间局部性**：当前被访问的数据地址附近的内存单元很快也会被访问。
* **组织结构**：以 ARM Cortex-M7 为例，其 D-Cache（数据缓存）通常采用 **4路组相联（4-way set associative）** 结构，支持高并发命中。

### 2. 核心工作策略
* **写策略（Write Policy）**：
  * **Write-Back（回写）**：CPU 写数据时仅写入 Cache，对应的 Cache Line 被标记为 **Dirty（脏）** 状态，并不立即写回主存。只有当该 Cache Line 被替换出（Evict）或由软件显式触发清理时，才写入主存。极大减少了总线交互开销，但带来了一致性隐患。
  * **Write-Through（直写）**：同时写入 Cache 和主存，一致性好但写带宽极低。
* **分配策略（Allocation Policy）**：
  * **Write-Allocate**：写缺失时，先将主存数据加载到 Cache 中，再在 Cache 中进行写操作。

---

## 二、 Cache 一致性缺失的根源

### 1. 多主设备并发访存冲突
在现代高性能 MCU（如 STM32H7、i.MX RT 等）系统中，除了 CPU 核心外，还存在大量的总线主设备（Bus Masters），例如 **DMA 控制器、以太网 MAC、USB DMA、硬件加密加速器**等。

### 2. 硬件架构层面的差异
* **缺乏硬件级一致性（No Hardware Coherency）**：
  * 在高端应用处理器（如 ARM Cortex-A 系列）中，通常配有 **SCU（Snoop Control Unit，窥探控制单元）** 或支持 AMBA CHI 协议的互联架构，能够自动监视其他主设备对内存的修改并自动使失效本地 Cache。
  * 在 MCU 领域（如 Cortex-M7），**完全没有硬件 Snoop 机制**。DMA 控制器直接读写物理 RAM，而 CPU 读写的是本地 D-Cache 或写缓冲区（Write Buffer）。两者的“内存视图”在硬件上是物理隔离的。

---

## 三、 三种应对方案的架构评估与工程权衡

| 方案编号 | 方案名称 | 核心实现 | 架构评估与适用场景 |
| :--- | :--- | :--- | :--- |
| **3a** | **CPU 直接关闭 Cache** | `SCB_DisableDCache()` | **致命的性能倒退**。强行关掉 Cache 会导致 CPU 访问 Flash/RAM 遇到严重的“存储墙”，执行效率暴跌 50% ~ 70% 以上。**严禁在量产产品中使用**，仅限极度诡异的底层排查阶段。 |
| **3b** | **MPU 配置为 Non-Cacheable** | 通过 MPU 将特定 RAM 划定为 `Non-Cacheable` 和 `Shareable` | **控制类、频繁小数据、核间通信的黄金标准**。CPU 绕过 Cache 直接读写物理 RAM。绝对安全，免去软件维护心智负担；但失去 Cache 加速，不适合大吞吐量计算。 |
| **3c** | **软件维护操作 (Clean / Invalidate)** | 传输前后显式调用 `SCB_CleanDCache_by_Addr` 或 `SCB_InvalidateDCache_by_Addr` | **大吞吐量外设 DMA 传输（如 Ethernet、USB、SDMMC、Audio DMA）的必经之路**。兼顾极致的处理性能与多主设备的数据一致性。 |

---

## 四、 深度剖析：MPU Non-Cacheable (方案 b) vs Cacheable + 软件维护 (方案 c) 的效率差异

在多核共享内存或 DMA 场景下，虽然最终数据都落在物理 RAM 中，但两者的**数据处理效率有天壤之别**：

### 1. 数据处理阶段的执行速度
* **方案 c（Cacheable + 软件维护）**：
  * **极速**：在数据处理循环中，CPU 对数据的密集读、写、计算全部命中 **L1 D-Cache**（延迟仅需 1 个 CPU 周期）。
  * **开销集中**：仅在数据处理前后产生一次性的 Cache 刷新开销。在密集运算场景下，能带来数倍甚至数十倍的性能飞跃。
* **方案 b（MPU Non-Cacheable）**：
  * **受限于 RAM 延迟**：CPU 的每一次 `LDR`/`STR` 指令都要穿透到慢速的物理 RAM 或共享总线上（数十甚至上百个周期），CPU 长期处于总线等待状态。

### 2. 系统总线带宽利用率
* **方案 b**：每次操作都在总线上发起单次读写，产生大量零散总线流量，极易引发**总线冲突（Bus Contention）**。
* **方案 c**：Cache 内部修改不占用总线流量，只有在执行 `Clean` 时，才通过高效的**突发传输（Burst Transfer）**一次性把整行（32字节）写回 RAM，总线利用率极高。

### 3. 选型决策准则
* **高频计算、低频通信**（如：大块音视频流、FFT、数字滤波、网络数据包解析）：**必须选方案 c**。
* **低频、零星的控制/状态交互**（如：核间 Mailbox 标志位、环形队列读指针、控制命令字）：**推荐选方案 b**，省去复杂的 Cache 维护逻辑和对齐误伤风险。

---

## 五、 工程红线与核心陷阱：Cache Line 边界对齐与“误伤”机制

在采用方案 c 进行软件维护时，90% 的隐蔽 Bug 都源于 **Cache Line 边界对齐失控**。

### 1. 硬件操作粒度与“误伤”原理
* 现代 MCU 的 Cache Line 大小为 **32 字节**。
* 当调用 `SCB_CleanDCache_by_Addr((uint32_t*)ptr, size)` 时，硬件操作的粒度是**以包含该地址的整个 32 字节块为单位**的。
* **危险场景模拟**：
  若变量 `A` 和变量 `B` 在内存中紧挨着，并刚好落在了**同一个 32 字节的 Cache Line** 内。若你在代码中仅修改了变量 `A` 并对其调用 Clean，硬件会把整个 32 字节块（包含未修改或由其他逻辑维护的变量 `B` 的旧状态）整体写回主存，从而**“误伤”变量 `B`**，造成极其诡异的逻辑错乱。

### 2. 开发铁律
1. **强制对齐**：所有用于 DMA 传输或多核共享的缓冲区，必须在定义时强制进行 32 字节对齐：
   ```c
   __attribute__((aligned(32))) uint8_t dma_tx_buffer[BUFFER_SIZE];