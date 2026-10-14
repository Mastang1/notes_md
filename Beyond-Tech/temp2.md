我已经完整阅读了 `ipc-shm.c`、`ipc-queue.c/.h`、`ipc-types.h` 的相关代码。下面是基于源码的分析结论。

---

# 一、Managed Channel 内存详细组成图

## 1. 关键数据结构（源码出处）

内存组成由三类结构决定（`ipc-shm.c` / `ipc-queue.h`）：

| 结构体 | 字段 | 大小 | 说明 |
|---|---|---|---|
| `struct ipc_shm_global` | `uint64 state` | 8 B | 位于整个共享内存起始，标志本地/远端是否就绪 |
| `struct ipc_ring` | `sentinel`(8) + `write`(4) + `read`(4) + `data[]` | 控制头 **16 B** | 内存映射环形缓冲 |
| `struct ipc_shm_bd` | `pool_id`(2) + `buf_id`(2) + `data_size`(4) | **8 B** | 缓冲区描述符（BD） |
| `struct ipc_queue` | `elem_num` + `elem_size` + `push_ring*` + `pop_ring*` | RAM 私有 | 双环无锁 FIFO |

> 关键公式：`ipc_queue_mem_size(queue) = sizeof(struct ipc_ring) + elem_num * elem_size`
> 其中 `elem_num` 已在 `ipc_queue_init()` 里 **+1**（多留 1 个 sentinel 元素保证无锁单生产者-单消费者安全）。
> 因此队列在共享内存中的实际占用 = **16 + (N+1) × 8** 字节（N 为元素个数）。

## 2. 整体共享内存布局（local / remote 对称，各一份）

```
local_shm 基地址 (本侧可写, 对侧只读)
+-----------------------------------+
| struct ipc_shm_global (8B)  state |  <- 全局就绪标志
+-----------------------------------+  <- chan_offset = 8
| Channel 0                         |
|   ...                             |
| Channel N-1                       |
+-----------------------------------+

remote_shm 基地址 (对侧可写, 本侧只读) —— 布局完全相同（对称）
+-----------------------------------+
| struct ipc_shm_global (8B)  state |
+-----------------------------------+
| Channel 0                         |
|   ...                             |
+-----------------------------------+
```

## 3. 单个 Managed Channel 的详细布局（字节级）

一个 managed channel 由「**1 个 channel 级发送队列**」+「**N 个 buffer pool**」组成。假设：1 个 pool、`num_bufs = 4`、`buf_size = 64`，则 `total_bufs = 4`。

### 本侧视角 —— local_shm 中的 channel 区域（本侧写）

```
Channel 起始
│
├─ [Channel 发送队列 push_ring]   ← 本侧 Tx ring（本侧写 BD）
│    offset 0:  sentinel         (8B)   = IPC_QUEUE_INIT_DONE
│    offset 8:  write            (4B)   写指针(本侧维护)
│    offset 12: read             (4B)   读指针(镜像对侧, 见下)
│    offset 16: data[(4+1)*8]    (40B)  共 total_bufs+1 个 BD 槽位
│    => 占用 16 + 40 = 56 B
│
├─ [Pool0 缓冲池队列 push_ring]   ← 本侧 release ring（本侧释放 BD）
│    sentinel(8) + write(4) + read(4) + data[(4+1)*8](40B)
│    => 占用 56 B
│
└─ [Pool0 buffers]               ← 本侧 buffer 数组（本侧写数据）
     buf[0] (64B)
     buf[1] (64B)
     buf[2] (64B)
     buf[3] (64B)
     => 占用 64*4 = 256 B

Channel 总占用 = 56 + 56 + 256 = 368 B
```

### 对侧视角 —— remote_shm 中的 channel 区域（本侧只读，对侧写）

```
Channel 起始
│
├─ [Channel 发送队列 pop_ring]    ← 本侧 Rx ring（本侧从这里 pop 对侧 push 的 BD）
│    sentinel(8) + write(4) + read(4) + data[(4+1)*8](40B)
│
├─ [Pool0 缓冲池队列 pop_ring]    ← 本侧 acquire ring（本侧 acquire 时从这里 pop BD）
│    sentinel(8) + write(4) + read(4) + data[(4+1)*8](40B)
│
└─ [Pool0 buffers]               ← 对侧 buffer 数组（对侧写数据）
     buf[0..3] (各 64B)
```

### 环（ring）的映射关系（双环对称，无锁关键）

```
       本侧队列                       对侧互补队列
  ┌──────────────────┐          ┌──────────────────┐
  │ push_ring(Tx)    │ ───────► │ pop_ring (Rx)    │   本侧 Tx ring == 对侧 Rx ring
  │ (写 local_shm)   │          │ (读 remote_shm)  │
  │ pop_ring (Rx)    │ ◄─────── │ push_ring (Tx)   │   本侧 Rx ring == 对侧 Tx ring
  │ (读 remote_shm)  │          │ (写 local_shm)   │
  └──────────────────┘          └──────────────────┘
```

- **写**永远只发生在自己的 `local_shm`（push_ring），**读**永远只读对方的 `local_shm`（即自己的 `pop_ring`），从而做到「免干扰（freedom from interference）+ 无锁 SPSC」。
- read/write 索引互换：pop 时本地只更新 `push_ring->read`，push 时本地只更新 `push_ring->write`，避免跨核写对方内存。

### 内存大小通用公式

```
channel 发送队列占用  = 16 + (total_bufs + 1) × 8
pool i 队列占用       = 16 + (num_bufs_i + 1) × 8
pool i 总占用         = pool i 队列占用 + buf_size_i × num_bufs_i
channel 总占用        = channel 发送队列占用 + Σ pool_i 总占用
total_bufs            = Σ num_bufs_i
```

对应代码：`managed_channel_init()` 建 channel 队列 → `managed_pools_init()` 从 `local_shm + queue_mem_size` 开始逐个放 pool → `ipc_buffer_populate()` 计算 `local/remote_pool_addr` 与 `shm_size`；`get_chan_memmap_size()` 汇总整 channel 大小。

## 4. RAM 私有数据结构（非共享内存，仅本侧 CPU 可见）

```
struct ipc_shm_priv[instance]
└── struct ipc_shm_channel channels[N]
    └── union ch
        └── struct ipc_managed_channel  (RAM 私有)
            ├── struct ipc_queue bd_queue   ← channel 发送队列(描述符指向共享内存)
            │     ├── elem_num / elem_size
            │     ├── push_ring → local_shm  (Tx ring)
            │     └── pop_ring  → remote_shm (Rx ring)
            ├── uint16 num_pools
            ├── struct ipc_shm_pool pools[MAX_POOLS=4]
            │     ├── num_bufs / buf_size / shm_size
            │     ├── local_pool_addr  → local_shm 中的 buffer 区
            │     ├── remote_pool_addr → remote_shm 中的 buffer 区
            │     └── struct ipc_queue bd_queue  ← pool 空闲缓冲队列
            │           ├── push_ring → local_shm  (release ring)
            │           └── pop_ring  → remote_shm (acquire ring)
            ├── rx_cb      (接收回调)
            └── cb_arg
```

---

# 二、发送队列的作用

「发送队列」即 `struct ipc_managed_channel` 里的 **`bd_queue`**（channel 级队列，`IPC_SHM_CHANNEL_QUEUE`）。它由两个环组成：

- **push_ring = 本侧 Tx ring（发送环）**
- **pop_ring = 本侧 Rx ring（接收环）**

它的作用可以概括为：

1. **承载「已发送/已接收缓冲区」的 BD（buffer descriptor）**，是消息数据在发送方与接收方之间传递的**信令通道**。BD 里只有 3 个信息：`pool_id`、`buf_id`、`data_size`，不搬运数据本体——数据本体就在共享内存的 buffer 区里，BD 只是"哪个池、第几号 buffer、装了多少字节"的指针。

2. **通过双环对称关系连接发送与接收**：本侧 Tx ring == 对侧 Rx ring，本侧 Rx ring == 对侧 Tx ring。发送方 `push` 一个 BD 进 Tx ring 并触发中断，接收方在 Rx 中断里 `pop` 这个 BD，据此定位 buffer、调用应用 Rx 回调。

3. **解耦生产者与消费者 + 无锁并发**：每个环只有一个写者（SPSC），发送和接收互不阻塞，天然支持"一方只写本地、一方只读远端"，保证实时性和免干扰。

4. **与 pool 级队列分工明确**：channel 发送队列管"**有数据要读**"（sent/received buffer），pool 队列管"**有空闲 buffer 可用**"（free buffer）。发送队列是**消息通知**路径，pool 队列是**缓冲回收/复用**路径。

---

# 三、发送队列的工作场景流程

以「本侧 A 发送数据给对侧 B」为例（对方向相反时完全对称）：

```
A 侧应用                 A 侧驱动(发送队列)              B 侧驱动                 B 侧应用
   │ 1. ipc_shm_acquire_buf()                          │
   │◄──────────────────────(返回本地空闲 buffer)         │
   │     (从 pool acquire ring pop 到空闲 BD)           │
   │ 2. 向 buffer 写入数据                              │
   │ 3. ipc_shm_tx(buf, size)                          │
   │─────────────────────►│                            │
   │                      │ find_pool_for_buf() 定位 pool│
   │                      │ 构造 BD{pool_id,buf_id,size} │
   │                      │ ipc_queue_push(channel bd_queue)│
   │                      │   → 写入 Tx ring(push_ring) │
   │                      │ flush dcache + irq_notify ──┼──► 中断
   │                      │                            │ 4. Rx 处理 ipc_shm_rx
   │                      │                            │    ipc_channel_rx()
   │                      │                            │    ipc_queue_pop(channel bd_queue)
   │                      │                            │    → 从 Rx ring(pop_ring) 取 BD
   │                      │                            │    定位 buffer 地址
   │                      │                            │    (pool->remote_pool_addr + buf_id*buf_size)
   │                      │                            │────► 调用 rx_cb(buf, size)
   │                      │                            │                     │ 5. 处理数据
   │                      │                            │◄────────────────────│ 6. ipc_shm_release_buf()
   │                      │                            │ find_pool_for_buf(remote)
   │                      │                            │ 构造 BD{pool_id,buf_id,0}
   │                      │                            │ ipc_queue_push(pool bd_queue)
   │                      │                            │   → 写入 release ring
   │  (A 的 acquire ring 又出现空闲 BD，可再次发送)      │
```

**逐步说明：**

1. **获取发送缓冲（不经过发送队列）**：A 调用 `ipc_shm_acquire_buf()`，从 pool 队列的 **acquire ring**（A 的 pop_ring = B 的 release ring）pop 出一个描述「A 侧本地 buffer」的 BD，返回 A 的 buffer 地址。这一步复用的是**缓冲池队列**。

2. **写数据**：A 直接往该 buffer 写入要发送的内容。

3. **发送（发送队列核心写入）**：A 调用 `ipc_shm_tx()` → `ipc_shm_buf_tx()`：
   - `find_pool_for_buf(..., IPC_BUFFER_FROM_LOCAL)` 反查 buffer 属于哪个 pool，算出 `pool_id`、`buf_id`；
   - 组装 `struct ipc_shm_bd {pool_id, buf_id, data_size=size}`；
   - `ipc_queue_push(&chan->bd_queue, &bd)` 把 BD 写入**发送队列的 Tx ring（push_ring）**；
   - `ipc_hw_flush_cache_local()` 刷 cache，`ipc_hw_irq_notify()` 触发对侧中断。

4. **接收（发送队列核心读取）**：B 在 Rx 中断/软中断里执行 `ipc_shm_rx()` → `ipc_channel_rx()`：
   - `ipc_queue_pop(&mchan->bd_queue, &bd)` 从**发送队列的 Rx ring（pop_ring = A 的 Tx ring）** pop 出 BD；
   - 用 `pool->remote_pool_addr + bd.buf_id * pool->buf_size` 解析出 B 视角下（位于 B 的 remote_shm）的 buffer 地址；
   - 校验地址合法后调用 `rx_cb(cb_arg, instance, chan_id, buf, bd.data_size)` 把 buffer 交给 B 的应用。

5. **B 应用处理数据**。

6. **释放缓冲（回到缓冲池队列，不经过发送队列）**：B 处理完调用 `ipc_shm_release_buf()`：
   - `find_pool_for_buf(..., IPC_BUFFER_FROM_REMOTE)` 定位 pool；
   - 组装 `BD{pool_id, buf_id, data_size=0}`；
   - `ipc_queue_push(&pool->bd_queue, &bd)` 写入 **release ring**（B 的 push_ring = A 的 acquire ring）。

至此，A 侧的 acquire ring 又出现空闲 BD，A 可以再次 acquire 该 buffer 发起下一轮发送，形成完整的**「acquire → 写 → tx(push 发送队列) → rx(pop 发送队列) → release(push 缓冲池队列)」**闭环。

---

## 小结

- **内存组成**：全局 8 B 头 + 每 channel 一个「发送队列（channel bd_queue）」+ 每 pool 一个「缓冲池队列（pool bd_queue）+ 若干 buffer」。发送队列和缓冲池队列都是「16 B 控制头 + (N+1)×8 B 的 BD 槽位」，buffer 区为 `buf_size × num_bufs`。
- **发送队列作用**：传递 `{pool_id, buf_id, data_size}` 的 BD，实现发送方→接收方的数据就绪通知与定位，是 managed channel 的消息信令通道；它与负责空闲 buffer 回收的 pool 队列配合，构成完整的数据搬运闭环。
- **场景流程**：acquire（pool 队列）→ 写数据 → tx（发送队列 push）→ 中断 → rx（发送队列 pop + 回调）→ release（pool 队列 push），周而复始。