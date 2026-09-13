# ARINC 825：从 CAN Data Link Layer 一步一步设计出完整航空 CAN 协议

> 目标：不是“背 ARINC 825 目录”，而是假设我们已经有一个能工作的 CAN 控制器，从 **Data Link Layer** 开始，随着工程问题出现，一步一步把 ARINC 825 的上层机制设计出来。每一步回答：**现在缺什么？为什么必须增加这个机制？ARINC 825 怎么规定？增加以后系统获得什么能力？**

## 0. 版本、边界与核对原则

本文以 **ARINC 825-4（2018-09-25，Current）** 为当前版本基线。SAE 官方说明 Supplement 4 将 CAN FD 纳入 ARINC 825，并新增四个附录主题：**ARINC 825 Compliance、Configuration of Bit Timing、MIB Counters、CAN Bus Security Considerations**。

严格性原则：

1. 29-bit Identifier、LCC、ATM、DMC、PTP、FID/DOC/NID、Bandwidth Management、High Integrity、Redundancy、Gateway、Communication Profile 等，只使用能够与当前版本资料交叉核对的结构。
2. **不再使用**此前错误的 `Priority + LCC + Destination + Service + Source` 自定义格式；它不是 ARINC 825。
3. 对公开资料没有完整暴露的规范表（例如某些 Node Service/PHSM 的完整服务码与 payload 定义），本文不会自行创造值，而标记为 **[需查规范表]**。
4. 若目标是“逐条 SHALL、逐个服务码、逐个保留值都与标准正文零差异”的认证级审查，必须以正版 ARINC 825-4 全文作为最终 normative source；本文不冒充受版权保护的规范正文。

---

# Part I：先列出全部知识点——最终要“造”出什么

## 1. Data Link Layer
- ISO 11898-1 compliance
- Classical CAN / CAN FD frame
- Extended 29-bit Identifier
- Data / Error / Remote / Overload frame
- Arbitration
- Bit stuffing
- CRC / ACK
- Hardware acceptance filtering
- Bit / Stuff / CRC / Form / ACK error
- TEC / REC
- Error Active / Error Passive / Bus Off
- Bus-off recovery / reattachment
- Performance and robustness

## 2. CAN Communication
- Communication Concept
- 29-bit Identifier architecture
- Logical Communication Channel (LCC)
- One-to-Many / ATM
- Directed Message
- Peer-to-Peer / PTP
- FID / Source FID / Client FID / Server FID
- FSB / LCL / PVT / DOC / SID / NID / SMT / RCI
- DMC Source/Destination Address
- DMC Source/Destination Port
- multicast
- interoperability
- big-endian / data types / units / axes/sign
- Periodic Health Status
- Node Service Interface
- bandwidth management
- High Integrity Protocol

## 3. System integration
- redundancy management
- gateway model / forwarding
- cross-network transfer
- gateway redundancy
- Communication Profile Database
- CAN FD
- Compliance
- Bit Timing configuration
- MIB counters
- security considerations

---

# Part II：工程场景

假设网络如下：

```text
          ARINC 825 CAN BUS
=====================================================
   |               |              |               |
Flight Control   Actuator-A     Actuator-B       CMS
   LRU              LRU            LRU        Maintenance
   |
 Gateway
   |
AFDX / other aircraft network
```

需求逐步增长：Flight Control 周期广播参数；故障时快速发布异常事件；CMS 访问指定 LRU；节点之间需要定向消息和维护服务；节点健康必须可监视；总线必须可做 worst-case timing；安全关键数据要有额外端到端完整性；系统有冗余源和 Gateway；最后全部接口必须形成受控 Communication Profile。

---

# Stage 1：CAN Data Link Layer 已经解决什么

先只使用 ISO 11898-1 CAN：

```c
can_send(id29, data, len);
can_recv(&id29, data, &len);
```

CAN 已经提供：帧定界、总线访问、非破坏性仲裁、CRC、ACK、自动重发、错误检测、错误隔离、多主共享。

因此 ARINC 825 不重新发明 CAN MAC/DLL；它在此之上解决航空系统级通信问题。

# Stage 2：锁定 Data Link Layer 的基本规则

ARINC 825 的正常上层通信使用 **29-bit Extended CAN Identifier**。公开 DLL 资料指出其基于 ISO 11898-1/CAN 2.0B extended frame；11-bit base frame 只在受约束的兼容/迁移场景中考虑。

Classical CAN Extended Data Frame：

```text
SOF
 │
 ▼
┌──────────── Arbitration Field ────────────┐
│ Base ID │ SRR │ IDE │ Extended ID │ RTR │
└───────────────────────────────────────────┘
                    │
                    ▼
            ┌─ Control Field ─┐
            │ reserved │ DLC  │
            └─────────────────┘
                    │
                    ▼
             Data 0..8 Byte
                    │
                    ▼
                 CRC Field
                    │
                    ▼
                 ACK Field
                    │
                    ▼
                   EOF → IFS
```

Classical CAN DLC 0..8 对应 0..8 byte payload。ARINC 825 真正要新增的是 **29-bit Identifier 的语义 + payload 的系统语义**。

# Stage 3：仲裁决定为什么 Identifier 不能随便分配

CAN 使用 dominant `0`、recessive `1`。多个节点同时发送时，从 Identifier 高位逐 bit 比较；第一个不同位上发送 `1` 的节点看到总线为 `0` 后退出仲裁。通常数值更小的 ID 优先级更高。

因此：

> Identifier 不只是“名字”，它同时参与实时调度。

如果维护帧随便获得更小 ID，就可能长期抢占控制帧。所以 ARINC 825 必须系统化规划 29-bit Identifier。

# Stage 4：CAN 只有一片广播空间——需要 Logical Communication Channel

==真实系统通信并不只有一种：==
- 异常事件
- 正常周期/非周期参数
- 指定节点消息
- 节点服务
- 测试维护
- 用户定义/迁移兼容

ARINC 825 使用 Identifier **最高 3 bit** 定义 LCC：

```text
bit28                              bit0
  │
  ▼
┌───────┬───────────────────────────────┐
│  LCC  │      Remaining ID fields      │
│ 3 bit │            26 bit             │
└───────┴───────────────────────────────┘
 28..26
```

关键：**LCC 不是额外 Priority 字段。** 它首先划分 Logical Communication Channel；因为处于 ID 最高 3 bit，它又自然成为整个 CAN 仲裁的第一层比较。

# Stage 5：LCC 的当前划分

| bits | LCC | 名称 | 主要通信模型 |
|---|---:|---|---|
| `000` | 0 | EEC — Exception Event Channel | ATM |
| `001` | 1 | Reserved | 保留 |
| `010` | 2 | NOC — Normal Operation Channel | ATM |
| `011` | 3 | DMC — Directed Message Channel | Directed Message |
| `100` | 4 | NSC — Node Service Channel | PTP |
| `101` | 5 | UDC — User-Defined Channel | 用户定义 |
| `110` | 6 | TMC — Test and Maintenance Channel | PTP |
| `111` | 7 | FMC — CAN Base Frame Migration Channel | 迁移/兼容用途 |

所以粗粒度仲裁天然按：

```text
000 EEC
  ↓
010 NOC
  ↓
011 DMC
  ↓
100 NSC
  ↓
101 UDC
  ↓
110 TMC
  ↓
111 FMC
```

`001` 保留，不应拿来做自定义业务。

# Stage 6：正常参数广播——为什么需要 ATM / One-to-Many

Flight Control 的一个参数可能同时给多个 consumer：

```text
               ┌── Actuator A
Flight Ctrl ───┼── Actuator B
               ├── Recorder
               └── Monitor
```

如果逐节点发送会重复占用带宽。CAN 天然是 broadcast medium，因此 ARINC 825 用 **One-to-Many / ATM** 发布参数。EEC 和 NOC 采用这种结构。

# Stage 7：完整设计 ATM Identifier

```text
MSB                                                         LSB
bit28                                                       bit0
 ┌─────┬─────────────┬─────┬─────┬─────┬────────────────┬─────┐
 │ LCC │ Source FID  │ FSB │ LCL │ PVT │      DOC       │ RCI │
 │  3  │      7      │  1  │  1  │  1  │       14       │  2  │
 └─────┴─────────────┴─────┴─────┴─────┴────────────────┴─────┘
 28-26     25-19       18    17    16       15-2           1-0
```

`3+7+1+1+1+14+2 = 29 bit`。

## 7.1 Source FID：数据属于哪个航空功能

ATM 的 `Source FID` 是 7 bit，标识消息来源的 aircraft function/subsystem，而不是简单 Node Address。示例公开分配包括：
- FID 10: Flight Controls
- FID 34: Landing Gear
- FID 102: Central Maintenance System
- FID 125: Periodic Health Status Message
- FID 127: Temporary Test & Maintenance

## 7.2 DOC：这个功能具体发布哪个 Data Object

同一个 FID 下有很多参数，因此再用 14-bit `DOC` 标识 Data Object。核心索引是：

```text
(Source FID, DOC)
        │
        ▼
Communication Profile
        │
        ▼
具体物理参数/数据语义
```

## 7.3 FSB：链路正确不等于业务有效

CAN CRC 只能证明传输层面的完整性，无法证明传感器本身是否已经故障。`FSB` 用于表达 payload 数据的 functional validity。

```text
link integrity != semantic validity
```

## 7.4 LCL：Gateway forwarding scope

`LCL=1` 表示消息只属于发送节点所在本地网络 segment，Gateway 不应把它转发到其他网络。

## 7.5 PVT：私有消息语义

`PVT` 标识只对特定实现有意义的 private message，使标准公共语义与 proprietary/private 语义显式分开。

## 7.6 RCI：冗余源身份

`RCI` 为最低 2 bit，最多表达四个 redundancy channels。它告诉接收端“这份同类数据来自哪个冗余通道”，但 **RCI 本身不是 voting/switchover 算法**。

# Stage 8：为什么 EEC 和 NOC 分开

正常运行参数：

```text
LCC = 010 NOC
```

快速异常/退化事件：

```text
LCC = 000 EEC
```

因为 `000 < 010`，EEC 在 CAN Identifier 的最高位阶段就优先于 NOC。这里不存在额外 `Priority=0` 字段；优先效果来自 **LCC 编码的位置**。

# Stage 9：Hardware Acceptance Filtering

所有节点都能物理看到广播帧，但不能让 CPU 对全部流量中断后再软件过滤。29-bit ID 的分层结构允许 CAN controller 依据 LCC/FID/对象范围做 hardware acceptance filter，减少 CPU load 和 jitter。

因此 Identifier 设计同时服务三件事：

```text
semantic organization
+ arbitration
+ hardware filtering
```

# Stage 10：只想给某一节点发消息——DMC 出现

ATM 解决 producer → many consumers，但 CMS 可能要明确给 Node 42 发一个定向请求。ARINC 825-4 使用：

```text
LCC = 011
DMC = Directed Message Channel
```

此前“Source/Destination/Port”记忆来自这里，而不是 ARINC 825 所有帧统一格式。

# Stage 11：完整设计 DMC Identifier

```text
MSB                                                        LSB
 ┌─────┬──────────────┬────────────────┬────────────┬──────────────┐
 │ LCC │Source Address│Destination Addr│Source Port │Destination   │
 │  3  │      7       │       7        │    6       │ Port ID  6  │
 └─────┴──────────────┴────────────────┴────────────┴──────────────┘
 28-26     25-19            18-12          11-6          5-0
```

其中 `LCC=011`。

DMC 需要每个 node 具有 7-bit unique address；地址 `0` 保留，不能分配为普通唯一节点地址，并用于 multicast 语义。

Port 为 6 bit：

| Port | 用途 |
|---:|---|
| 0..23 | Ephemeral port，dialog 临时使用 |
| 24..31 | Reserved well-known ports |
| 32..47 | Reserved for future definition |
| 48..63 | User-defined functions |

# Stage 12：DMC Dialog 为什么需要 Port

只有 `Node17 → Node42` 还不知道 Node42 的哪项逻辑服务，也无法区分多个并发 conversation。因此 DMC 使用：

```text
(src node, src port) -> (dst node, dst port)
```

发起方第一条消息：
- Source Address = 发起节点
- Destination Address = 目标节点
- Source Port = 发起方 ephemeral port
- Destination Port = 对应目标功能/WKP

若形成双向 dialog，对端在响应中建立自己的 ephemeral port，后续双方通过端口对标识 dialog。结束后 ephemeral port 可复用。

若 `Destination Address=0`，DMC 可用于 multicast service。

# Stage 13：还需要标准化 Client/Server Node Service——PTP 出现

系统还需要查询、维护、数据加载、节点 interrogation 等明确的 client/server service model，因此 ARINC 825 还定义 **Peer-to-Peer / PTP**，主要对应 NSC/TMC。

完整 PTP Identifier：

```text
MSB                                                            LSB
 ┌─────┬────────────┬─────┬─────┬─────┬────────────┬─────┬─────┐
 │ LCC │ Client FID │ SMT │ LCL │ PVT │ Server FID │ SID │ RCI │
 │  3  │      7     │  1  │  1  │  1  │      7     │  7  │  2  │
 └─────┴────────────┴─────┴─────┴─────┴────────────┴─────┴─────┘
 28-26     25-19      18    17    16       15-9       8-2   1-0
```

对比：

```text
ATM:
LCC | Source FID | FSB | LCL | PVT | DOC        | RCI

PTP:
LCC | Client FID | SMT | LCL | PVT | Server FID | SID | RCI
```

ARINC 825 不是用一张统一字段表解决所有通信，而是按 communication model 重解释 Identifier。

## 13.1 NID：具体 server 的身份

`Server FID + SID` 提供 node/server addressing 语义，结合 redundancy context 形成 Node Identifier 概念。FID 0 与 SID 0 保留用于 multicast 语义。

## 13.2 SMT：Request / Response 方向

PTP 中 bit18 不再是 ATM 的 FSB，而是 `SMT`：
- Node Service request：SMT set
- Node Service response：SMT clear

所以 bit18 的意义取决于通信模型，绝不能把 ATM 字段机械套到 PTP。

## 13.3 Connectionless / Connection-oriented

PTP Node Service 支持 connectionless 与 connection-oriented（handshake）交互思想，可帮助类比 UDP/TCP，但 ARINC 825 并未实现 IP/TCP stack。

完整 Node Service service code / payload 表：**[需查 ARINC 825-4 normative table；不自行创造编号]**。

# Stage 14：Periodic Health Status——需要统一节点健康监视

只看某个业务 DOC 是否更新，无法等价判断整个节点健康。ARINC 825 定义 Periodic Health Status；公开 FID 分配中：

```text
FID 125 = Periodic Health Status Message (PHSM)
```

它把节点整体 health monitoring 从单个业务参数中抽离出来。PHSM 完整 payload/status code：**[需查 ARINC 825-4 normative PHSM table，不猜测保留位]**。

# Stage 15：Interoperability——相同 bytes 不代表相同物理量

两个供应商都发送 `00 00 27 10`，一个解释为 ft，另一个解释为 m，CAN CRC 完全发现不了。

因此 ARINC 825 还规范：
- big-endian only
- Boolean / Integer / Floating-Point 等数据类型
- physical units
- aeronautical axes/sign conventions
- aircraft function definitions

这是更高层的 data representation / interoperability 问题。

# Stage 16：Communication Profile——把 FID+DOC 变成真正的数据合同

`FID+DOC` 只是索引。真正数据语义还需要：

```text
name
data type
unit
range
scaling
repetition/update rate
validity
source/consumer
gateway behavior
timing
```

因此：

```text
FID + DOC
    │
    ▼
Communication Profile
    │
    ├── semantic meaning
    ├── data type/range/scaling
    ├── transmission behavior
    └── interoperability contract
```

Communication Profile 是系统接口定义的重要组成。

# Stage 17：节点多了以后，仲裁并不能自动保证实时性

大量周期帧 + event + node service + maintenance 会导致低优先级消息长期等待：

```text
arbitration != bandwidth engineering
```

必须设计 Bandwidth Management。

Classical extended CAN 在 worst-case stuffing 下的公开 ARINC 825 frame length：

| Payload | Max frame bits |
|---:|---:|
| 0 B | 79 |
| 1 B | 88 |
| 2 B | 99 |
| 3 B | 109 |
| 4 B | 119 |
| 5 B | 128 |
| 6 B | 139 |
| 7 B | 149 |
| 8 B | 158 |

因此 8-byte payload 不能只按 64 bit 算占用。

基本负载思想：

```text
Load =
Σ(MessageLength × NumberOfMessages)
──────────────────────────────────
TransmissionInterval × DataRate
```

必须包含 periodic 和 aperiodic traffic，并使用能反映 worst-case 的 occurrence 与 stuffing。

# Stage 18：Minor Frame / Major Frame

ARINC 825 给出一种可接受的 bandwidth management 方法：

```text
Major Frame
├── Minor Frame 0
├── Minor Frame 1
├── Minor Frame 2
└── ...
```

- Major Frame：全部周期消息至少发送一次所覆盖的时间周期。
- Minor Frame：最高频消息发送一次的基本周期。
- 所有节点遵守相同 minor/major-frame timing concept。
- 每个节点在一个 minor frame 内不能超过系统为它分配的 transmission budget。
- 不要求所有节点每个发送 instant 严格全局同步。

公开 ARINC 825 bandwidth 资料推荐保留约 50% safety margin，因此正常运行平均 bus load 通常不应连续超过约 50%；对极小 transmit jitter 的实现可考虑约 30%。

目标不是把 bus 压到 99%，而是获得：

```text
predictability
fault margin
future growth
certifiability
```

# Stage 19：CAN CRC 为什么还不够——High Integrity Protocol

CAN CRC 是 link-level error detection，但安全关键系统还关心丢失、重复、顺序错误以及消息身份与应用路径完整性：

```text
CAN CRC != end-to-end integrity
```

Classical CAN High Integrity Message：

```text
┌──────────────────────────┬──────────┬──────────────┐
│ Application Data         │   SNo    │     MIC      │
│ up to 5 bytes            │  1 byte  │    2 bytes   │
└──────────────────────────┴──────────┴──────────────┘
```

SNo 为 8 bit。公开 ARINC 825-4 研究资料描述：首个序号为 0，随后逐次增加，255 后回到 1，用于发现 lost/duplicate/out-of-sequence。

MIC 为 16-bit CRC，覆盖：

```text
29-bit CAN ID + Application Data + SNo
```

FAA/Koopman 的 ARINC-825 MIC 摘要给出更正后的 polynomial：

```text
x^16 + x^15 + x^14 + x^11 + x^6 + x^5 + x^3 + x^2
```

即 `0xC86C`，并给出：
- initial CRC `0xFFF`
- 每个输入 byte reflect
- final CRC reflect
- final XOR `0xFFF`
- CAN ID 按 32 bit 输入，最高 3 bit 为 0
- `"0123456789"` test string 应得到 `0xC405`

注意：ARINC 825 早期 High Integrity CRC 公开资料曾存在歧义/错误，不能随便从旧博客/专利抄另一套 polynomial。

# Stage 20：CAN FD / ARINC 825-4

Supplement 4 纳入 CAN FD。CAN FD 支持最大 64-byte data field，arbitration phase 与 data phase 可使用不同 bit rate。SAE 对 825-4 的官方说明称其潜在提供约 4 Mbps data transfer。

ARINC 825-4 High Integrity Message 在 CAN FD 下可形成：

```text
Application Data 0..61 bytes
+ SNo 1 byte
+ MIC 2 bytes
<= 64 bytes
```

CAN FD 提升 payload 与吞吐，但不会消除 LCC、ATM/DMC/PTP、FID/DOC/NID、health、bandwidth、redundancy、gateway、profile 这些高层问题。

# Stage 21：Redundancy Management

如果同一逻辑变量有 A/B/C 冗余源：

```text
Source-A ─┐
Source-B ─┼─> same logical object
Source-C ─┘
```

RCI 只提供 redundancy identity。完整冗余管理还要求：
- TX 设置与 source/bus 对应的 RCI
- RX 检查各 RCI channel 的有效数据
- freshness checking
- active source selection
- backup channel monitoring
- failover handling
- 接受冗余数据可能因 arbitration/retry 出现到达时间偏斜

具体 voting/switchover threshold 属于 system safety/reliability design，不由 RCI 自动完成。

# Stage 22：Gateway

当网络变为：

```text
CAN-A ── Gateway ── AFDX / CAN-B
```

必须解决 forwarding、bandwidth mismatch、semantic mapping、fault containment、redundancy path 等问题。ARINC 825 官方 Gateway 章节覆盖：
- general requirements
- gateway model
- primary gateway functions
- gateway resources
- data transfer via gateway
- gateway redundancy management
- gateways of interest

`LCL=1` 在这里形成闭环：Gateway 接收到 local-only message 后不应把它转发到其他 network segment。

# Stage 23：Communication Profile Database——最终系统果实

网络中已经存在：

```text
LCC / FID / DOC
DMC node address / ports
PTP SID/NID
RCI
payload format
data types / units
cycle times
bus budget
gateway forwarding
redundancy
high integrity
```

若这些散落在头文件、Excel、Gateway 配置和 Word 中，网络没有 single source of truth。

Communication Profile Database 应能回答：

```text
有哪些消息？
谁发送/谁消费？
哪个 LCC？
FID / DOC / SID / port 是多少？
payload 如何解释？
单位/范围/scale 是什么？
周期/最大时延是什么？
是否 High Integrity？
是否冗余？
是否可经 Gateway 转发？
```

它同时是 interface contract、interoperability definition、verification input、bus-load-analysis input、test-tool input、gateway/configuration input。

# Stage 24：Data Link Error State 不能漏

CAN controller 至少有：

```text
Error Active
Error Passive
Bus Off
```

公开资料给出的典型阈值：
- Error Active：REC <=127 且 TEC <=127
- Error Passive：REC>=128 或 TEC>=128，并且 TEC<=255
- Bus Off：TEC>255

Bus-Off 的目的是把持续制造错误的节点逻辑隔离，避免拖死整个网络。

Bus-Off reattachment 按 ISO 11898-1 要求 REC/TEC 回到 0，并观察 128 次“11 个连续 recessive bits”。ARINC 825 robustness guidance 还要求系统设计 application-triggered normal-mode request、reattachment timeout、允许重接次数限制。

因此：

```text
hardware recovered != system automatically trusts node forever
```

# Stage 25：完整发送路径

```text
Application value
    ↓
ARINC 825 data representation
(big-endian / datatype / unit / scale)
    ↓
Communication Profile
    ↓
选择 communication model
ATM / DMC / PTP
    ↓
组装 29-bit Identifier
    ↓
可选 High Integrity SNo/MIC
    ↓
Bandwidth scheduler
    ↓
CAN Data Link Layer
(arbitration / CRC / ACK / retry / error state)
    ↓
Physical bus
```

# Stage 26：完整接收路径

```text
Physical bus
    ↓
CAN controller
    ↓
CRC / ACK / error handling
    ↓
hardware acceptance filter
    ↓
29-bit ID decode
    ↓
LCC
    ↓
ATM / DMC / PTP parser
    ↓
FID/DOC or address/port or NID
    ↓
FSB/LCL/PVT/RCI/SMT
    ↓
Communication Profile lookup
    ↓
big-endian decode
    ↓
unit / scale / validity / freshness
    ↓
Application
```

# Stage 27：实现模块边界

下面是推荐的软件职责划分，不把软件文件名冒充标准术语：

```text
+----------------------------------------------------------+
| Application                                               |
+----------------------------------------------------------+
| Communication Profile / Data Representation               |
+----------------------------------------------------------+
| ARINC 825 Communication                                   |
|   - LCC                                                   |
|   - ATM                                                   |
|   - DMC                                                   |
|   - PTP / Node Service                                    |
|   - PHSM                                                  |
|   - High Integrity                                        |
|   - Redundancy semantics                                  |
+----------------------------------------------------------+
| Traffic / Bandwidth Management                            |
+----------------------------------------------------------+
| CAN Data Link Layer                                       |
|   - frame / arbitration / filter / error-state            |
+----------------------------------------------------------+
| ISO 11898 Physical                                        |
+----------------------------------------------------------+
```

推荐不要实现一个错误的统一 ID：

```c
struct generic_a825_id {
    priority;
    dst;
    src;
    service;
};
```

而应分别实现 ATM/DMC/PTP pack/unpack，因为三种 Identifier 结构不同。

# Stage 28：整个协议为什么一步一步长成这样

```text
ISO 11898 CAN DLL
    │
    │ 已解决 frame/arbitration/CRC/ACK/error confinement
    ▼
“CAN ID 不能随便分”
    ▼
29-bit Identifier Architecture
    ▼
“不同通信目的需要隔离”
    ▼
LCC
    ├─────────────┬───────────────┐
    ▼             ▼               ▼
   ATM           DMC             PTP
    │             │               │
    │             │               └─ Client/Server + Node Service
    │             └─ src/dst address + ports + dialog
    ├─ Source FID
    ├─ FSB / LCL / PVT
    ├─ DOC
    └─ RCI
    ▼
“bytes 必须具有一致语义”
    ▼
Interoperability + Communication Profile
    ▼
“需要统一节点健康”
    ▼
Periodic Health Status
    ▼
“节点越来越多，worst-case 时延不可控”
    ▼
Bandwidth Management
    ▼
“CAN CRC 不等于端到端完整性”
    ▼
High Integrity Protocol
    ▼
“同类数据有多个冗余源”
    ▼
Redundancy Management
    ▼
“出现多个网络域”
    ▼
Gateway Considerations
    ▼
“所有定义必须成为受控系统合同”
    ▼
Communication Profile Database
```

---

# Part III：ARINC 825-4 / Supplement 4 不可遗漏项

SAE 官方说明当前 ARINC 825-4：
1. 增加 CAN FD；
2. 官方描述 CAN FD potential data transfer 约 4 Mbps；
3. 新增 ARINC 825 Compliance appendix；
4. 新增 Configuration of Bit Timing appendix；
5. 新增 MIB Counters appendix；
6. 新增 CAN Bus Security Considerations appendix。

这些 appendix 不是新的“OSI 网络层”，而是工程一致性、配置、运维可观测性和安全设计的 cross-cutting concern。

---

# Part IV：覆盖审计

按公开 ARINC 825 主目录反向核对：

## Data Link Layer
- [x] ISO compliance
- [x] frame types
- [x] arbitration
- [x] hardware acceptance filtering
- [x] error handling
- [x] node state machine
- [x] performance/robustness

## CAN Communication
- [x] Communication Concept
- [x] CAN Identifier Usage
- [x] LCC
- [x] ATM
- [x] DMC
- [x] PTP
- [x] Source/Client/Server FID
- [x] FSB / SMT / LCL / PVT / DOC / SID/NID / RCI
- [x] DMC address / port / dialog / multicast
- [x] interoperability
- [x] big endian / datatype / unit / axis
- [x] Periodic Health Status concept
- [x] Node Service Interface concept
- [x] Bandwidth Management
- [x] High Integrity Protocol

规范正文才能逐项零差异核对的 code tables：
- [规范表] PHSM 完整 payload/status code
- [规范表] Node Service 完整 service code/payload
- [规范表] 全量 FID assignment
- [规范/项目 profile] DOC assignment

## Gateway / Design
- [x] gateway purpose/model/forwarding
- [x] redundancy management
- [x] bus-load management
- [x] safety/reliability awareness
- [x] Communication Profile

## Supplement 4
- [x] CAN FD
- [x] Compliance
- [x] Bit Timing appendix
- [x] MIB Counters
- [x] Security Considerations

---

# Part V：三个最容易讲错的点

## 1. LCC 不是 Priority 字段

错误：

```text
Priority 3bit + LCC 4bit
```

正确：

```text
bit28..26 = LCC = 3bit
```

LCC 位于 Identifier MSB，所以首先参与 CAN arbitration。

## 2. Source/Destination/Port 不是统一 ARINC 825 格式

它是 **DMC**：

```text
LCC | Source Address | Destination Address | Source Port | Destination Port
```

ATM 与 PTP 使用另外两套字段结构。

## 3. 老资料里的 LCC3 Reserved 已过时

旧演示资料可能显示 `LCC3=Reserved`。当前 825-4 体系中：

```text
LCC3 = DMC
```

同理，旧资料可能把 ATM bit18 标成 reserved；当前结构使用 `FSB`。学习时必须带版本号。

---

# Part VI：是不是遵循 OSI 七层？

**ARINC 825 参考 OSI 的职责分层思想，但并不完整实现七层，也不应该为了“凑七层”强行造模块。**

ISO 11898 CAN 主要解决：
- Layer 1：Physical
- Layer 2：Data Link

ARINC 825 在其上补充部分更高层能力。公开 ARINC 825 技术资料明确将其描述为加入部分 Layer 3、Layer 4、Layer 6 功能，以支持 LCC、ATM/PTP、node addressing 和 data representation。

所以 OSI 的正确用途是：

> **separation-of-concerns reference model**

而不是：

> **所有总线协议都必须按 1→2→3→4→5→6→7 实现七个软件层。**

# 最终总结：通用协议设计思想（一段）

设计一套总线协议时，真正的方法不是机械地把 OSI 七层各实现一次，而是先确定底层媒介和链路层已经解决了什么，再从系统需求中逐项发现“尚未解决的问题”：如何标识消息和通信域、如何利用底层仲裁形成优先级、采用广播还是定向/请求响应、如何寻址节点与服务、如何描述 payload 的数据类型/单位/字节序、是否需要分段或会话、如何管理周期/带宽/最坏时延、如何判断节点健康和错误状态、如何表达冗余、如何实现端到端完整性、是否需要网关与跨网络转发、如何处理安全与诊断，最后如何用统一的配置/通信数据库把所有接口、时序和语义固化成可验证的系统合同；**OSI 是帮助检查职责边界的参考框架，而不是协议设计的强制施工顺序。** ARINC 825 正是这一思想的典型例子：它保留 ISO 11898 的物理层和数据链路层，只增加航空 CAN 系统真正需要的 LCC、ATM/DMC/PTP、互操作性、Node Service、Health、Bandwidth、High Integrity、Redundancy、Gateway 和 Communication Profile 等能力。

---

# References

1. SAE ARINC825-4, Current, 2018-09-25  
   https://saemobilus.sae.org/standards/arinc825-4-825-4-general-standardization-controller-area-network-bus-protocol-airborne-use

2. ARINC 825:2015 public table of contents, superseded by 825-4  
   https://shop.standards.ie/en-ie/standards/arinc-825-2015-98639_saig_arinc_arinc_207412/

3. ARINC 825 Data Link Layer  
   https://mil-std-1553.jp/825_p03.html

4. ARINC 825 Communication  
   https://mil-std-1553.jp/825_p04.html

5. ARINC 825 Bandwidth Management  
   https://mil-std-1553.jp/825_p05.html

6. ARINC 825 Redundancy Management  
   https://mil-std-1553.jp/825_p06.html

7. FAA/Koopman, ARINC-825 MIC excerpts / CRC  
   https://users.ece.cmu.edu/~koopman/lectures/2013_faa_crc.pdf

8. McGill University, ARINC 825-4 identifier / HIM research  
   https://escholarship.mcgill.ca/downloads/x059cd56x

9. McGill University, ARINC 825-4 CAN FD HIM  
   https://escholarship.mcgill.ca/downloads/8049g7390.pdf

10. esd ARINC 825 Library Manual, CAN FD support for ARINC825-4  
    https://esd.eu/fileadmin/esd/docs/manuals/ARINC825_Library_Manual_en_16.pdf
