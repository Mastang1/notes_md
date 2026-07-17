
# IPCS 异构多核通信测试操作指南 (CLI版 v3.0)

## 一、 测试环境与命令语法说明

测试平台所有控制命令均通过 **Cortex-M7_0 的串口** 统一输入。底层控制框架会解析 `tcf@` 协议，并通过内部 IPC 路由将命令分发到对应的核心执行。

### 1. 统一命令语法格式

所有输入的测试命令必须遵循以下协议包裹格式：

Plaintext

```
tcf@core<x>.IPCS:<API_Name>(<arg1>, <arg2>, ...)
```

- **`tcf@`**: 协议解析头。
    
- **`core<x>`**: **命令路由终点**（即该命令在哪颗核心上执行）。
    
    - `core0` = Cortex-M7_0 (核心别名: Metha)
        
    - `core1` = Cortex-M7_1 (核心别名: Propa)
        
    - `core4` = Cortex-A53 (核心别名: Penta)
        
- **`IPCS:`**: 模块标识符。
    
- **`<API_Name>`**: 调用的具体函数名及其参数。
    

### 2. 逻辑通信 ID (`dstCore` 参数)

API 参数中的 `dstCore` 代表**数据通信的对端目标**，映射关系固定如下：

- `0` = Metha (CM7_0)
    
- `1` = Propa (CM7_1)
    
- `2` = Penta (A53)
    

## 二、 基础命令速查表

|**分类**|**完整命令结构示例 (以发送至 A53 执行为例)**|**功能描述**|**参数说明**|
|---|---|---|---|
|**连通性**|`tcf@core4.IPCS:hello_a53()`|打印 hello 消息，测试框架自身连通性|无|
|**链路管理**|`tcf@core4.IPCS:ipcs_test_init(dstCore)`|初始化当前核到 `dstCore` 的 IPC 通信上下文|`dstCore`: 对端逻辑 ID (0,1,2)|
||`tcf@core4.IPCS:ipcs_test_deinit(dstCore)`|释放两核间通信上下文结构体变量及资源|同上|
||`tcf@core4.IPCS:ipcs_test_reset(dstCore)`|重置上下文，清空收发计数、错误计数与 Log|同上|
|**模式控制**|`tcf@core4.IPCS:ipcs_test_set_mode(mode)`|设置**当前核心**接收数据后的处理模式|`0`: VERIFY (CRC及序列校验)<br><br>  <br><br>`1`: ECHO (同通道回传)<br><br>  <br><br>`2`: UNM_THRUPUT (非管理模式压测接收)<br><br>  <br><br>`3`: MNG_THRUPUT (管理模式压测接收)|
|**数据收发**|`tcf@core4.IPCS:ipcs_test_unmanaged_send(dstCore, dataLen)`|通过**非管理通道**发送单次数据|`dataLen`: 1-1024 字节|
||`tcf@core4.IPCS:ipcs_test_managed_send(dstCore, chanId, dataLen)`|通过**管理通道**发送单次数据|`chanId`: 1-3 逻辑通道<br><br>  <br><br>`dataLen`: 字节数|
|**压力测试**|`tcf@core4.IPCS:ipcs_test_unmanaged_throughput(dstCore, durationSec)`|发起非管理通道 Burst/吞吐量压测|`durationSec`: 时长(1-3600秒)|
||`tcf@core4.IPCS:ipcs_test_managed_throughput(dstCore, chanId, durationSec, bufLen)`|发起管理通道 Burst/吞吐量压测|`chanId`: 1-3<br><br>  <br><br>`bufLen`: 8-256 字节|
|**统计信息**|`tcf@core4.IPCS:ipcs_test_get_info(dstCore)`|打印 TX/RX 计数、错误统计及 RTT 信息|同上|

## 三、 典型测试场景用例 (Copy & Paste)

### 场景 1：往返延迟测试 (Round Trip Time, RTT) & Ping-Pong

> 🛑 **前置要求**：为了获取准确的往返延迟数据，**该测试必须由 `core0` 发起**，并且通信双方都应配置为 `ECHO` 模式。

#### 用例 A：`core0` (M7_0) ↔ `core1` (M7_1) RTT 测试

Bash

```
# 1. 链路初始化与上下文重置
tcf@core0.IPCS:ipcs_test_init(1)
tcf@core1.IPCS:ipcs_test_init(0)
tcf@core0.IPCS:ipcs_test_reset(1)
tcf@core1.IPCS:ipcs_test_reset(0)

# 2. 将双方模式同时设置为 ECHO (模式 1)
tcf@core0.IPCS:ipcs_test_set_mode(1)
tcf@core1.IPCS:ipcs_test_set_mode(1)

# 3. 发起端 (core0) 发送数据包激活 Ping-Pong 测试 (通道1，256字节)
tcf@core0.IPCS:ipcs_test_managed_send(1, 1, 256)

# 4. 在发起端读取统计信息，查看 Round Trip Time (RTT) 结果
tcf@core0.IPCS:ipcs_test_get_info(1)

# 5. 释放通信上下文变量
tcf@core0.IPCS:ipcs_test_deinit(1)
tcf@core1.IPCS:ipcs_test_deinit(0)
```

#### 用例 B：`core0` (M7_0) ↔ `core4` (A53) RTT 测试

Bash

```
# 1. 链路初始化与上下文重置
tcf@core0.IPCS:ipcs_test_init(2)
tcf@core4.IPCS:ipcs_test_init(0)
tcf@core0.IPCS:ipcs_test_reset(2)
tcf@core4.IPCS:ipcs_test_reset(0)

# 2. 将双方模式同时设置为 ECHO (模式 1)
tcf@core0.IPCS:ipcs_test_set_mode(1)
tcf@core4.IPCS:ipcs_test_set_mode(1)

# 3. 发起端 (core0) 发送数据包激活 Ping-Pong 测试
tcf@core0.IPCS:ipcs_test_managed_send(2, 1, 256)

# 4. 在发起端读取统计信息，查看往返延迟
tcf@core0.IPCS:ipcs_test_get_info(2)

# 5. 释放通信上下文变量
tcf@core0.IPCS:ipcs_test_deinit(2)
tcf@core4.IPCS:ipcs_test_deinit(0)
```

### 场景 2：数据完整性校验测试 (VERIFY Mode)

Bash

```
# 1. 链路初始化与计数器重置
tcf@core4.IPCS:ipcs_test_init(0)
tcf@core0.IPCS:ipcs_test_init(2)
tcf@core4.IPCS:ipcs_test_reset(0)
tcf@core0.IPCS:ipcs_test_reset(2)

# 2. 将接收端 (M7_0) 设置为 VERIFY 模式 (模式 0)
tcf@core0.IPCS:ipcs_test_set_mode(0)

# 3. 发送端 (A53) 发送测试数据
tcf@core4.IPCS:ipcs_test_managed_send(0, 1, 256)

# 4. 检查接收端 (M7_0) 的收包状态以及是否存在 CRC/序列错误
tcf@core0.IPCS:ipcs_test_get_info(2)

# 5. 释放通信上下文
tcf@core4.IPCS:ipcs_test_deinit(0)
tcf@core0.IPCS:ipcs_test_deinit(2)
```

### 场景 3：管理通道 Burst / 吞吐量压测 (Managed Throughput)

Bash

```
# 1. 链路初始化与重置
tcf@core4.IPCS:ipcs_test_init(0)
tcf@core0.IPCS:ipcs_test_init(2)
tcf@core4.IPCS:ipcs_test_reset(0)
tcf@core0.IPCS:ipcs_test_reset(2)

# 2. 【核心步骤】将接收端 (M7_0) 设为 MNG_THRUPUT (模式 3)
tcf@core0.IPCS:ipcs_test_set_mode(3)

# 3. 发起端 (A53) 发起持续 5 秒，使用 32 字节 buffer 的 Burst 压测 (通道 1)
tcf@core4.IPCS:ipcs_test_managed_throughput(0, 1, 5, 32)

# 4. 压测结束后，分别获取双方统计数据 (重点关注 RX 计数是否符合预期)
tcf@core4.IPCS:ipcs_test_get_info(0)
tcf@core0.IPCS:ipcs_test_get_info(2)

# 5. 测试结束，恢复默认模式并释放通信上下文
tcf@core0.IPCS:ipcs_test_set_mode(0)
tcf@core4.IPCS:ipcs_test_deinit(0)
tcf@core0.IPCS:ipcs_test_deinit(2)
```

## 四、 核心机制与测试注意事项（测试必读）

- **关于 `THRUPUT` (吞吐量) 与 Burst 测试机制**：
    
    框架提供的 `UNM_THRUPUT` 和 `MNG_THRUPUT` 接口，在底层实质上采用了 **Burst (突发)** 测试方式。在高速连续发送数据的过程中，底层驱动自带了**背压（Backpressure）处理机制**。当接收端处理不及或共享内存队满时，发送端会进行合理的阻塞或退让，以防止内存被打爆导致系统崩溃。因此，**这两个吞吐量测试命令既属于常规的 Throughput 带宽评估用例，也完全适用于高频次、大批量的 Burst 抗压测试场景**。
    
- **为何必须调用 `ipcs_test_reset(dstCore)`**：
    
    由于每个 IPC 通信链路在内存中都维护着一个独立的测试上下文结构体（其中包含了接收个数、失败包数、历史 Log 缓冲等重要状态），强烈建议在**每执行一个全新测试场景前**调用一次 `reset` 命令。这能清空上一次测试的残留数据（脏数据），确保当前 RTT 或丢包率的统计绝对干净、准确。
    
- **关于 `ipcs_test_deinit(dstCore)` 接口的作用**：
    
    该接口的根本目的是**释放**此前由 `init` 初始化的两个核心之间的通信上下文结构体变量。如果不执行 `deinit` 直接退出或进入下一个重度测试，极易导致共享内存池泄漏或通信链路状态机卡死。因此，标准的测试规范必须是 `init` 建立结构体，测试完毕后 `deinit` 释放结构体。