# DDS 与 ROS 2 底层通信核心机制笔记

## Q1：DDS 协议的核心分层是什么？各自负责什么？
DDS 严格分为上下两层，以实现语义意图与网络传输的彻底解耦：
* **DCPS 层 (数据中心层)：** 面向开发者的业务层。负责管理 DomainParticipant、Publisher/Subscriber、DataWriter/DataReader 和 Topic，存储 QoS 契约，执行数据序列化。它只表达“发什么数据、要求什么质量”，完全不感知网络栈。
* **RTPS 层 (有线协议层)：** 面向网线的状态机引擎层。不认识业务结构体，只认全局唯一标识 `GUID_t` 和二进制序列流。它通过操作 HistoryCache（历史缓存），并收发 `HEARTBEAT`/`ACKNACK` 报文，确保本地缓存与远端缓存的精准同步。

## Q2：两个参与者同处一个 Topic，底层是如何建立点对点通信的？
分为两个阶段（基于 SPDP 和 SEDP 协议）：
1. **控制面阶段 (基于 Topic 字符串的媒人撮合)：** 
   双方通过 SEDP 协议互相单播交换元数据（包含自己期望的 `Topic` 名称和单播/多播 Locator 地址）。本地节点收到后，查找本地对应的 `DataWriter` 或 `DataReader`。一旦 Topic 字符串和 QoS 匹配成功，控制面在底层各自实例化一个 **`Proxy` 代理对象**（如 `ReaderProxy`），代理对象中保存了对方真实的物理 IP 和端口。
2. **数据面阶段 (绕过 Topic 的点对点直连)：**
   当真正发送数据时（`write` 调用），上层不再传递 Topic 字符串。`DataWriter` 直接通知其 1:1 绑定的底层 `RTPSWriter`。`RTPSWriter` 直接遍历自身下挂的所有 `ReaderProxy` 对象，提取 IP 和端口，调用操作系统底层的 Socket (`sendto`) 直接发包。此时 Topic 的使命已完成，底层本质是端到端的直连。

## Q3：ROS 2 里的中间件层 (Middleware) 是属于 ROS 还是 DDS 的？
**属于 ROS 层。** 
ROS 2 的设计哲学是“不被任何单一 DDS 厂商绑架”。这个中间件层全称叫 **RMW (ROS Middleware Interface)**，它是 ROS 官方定义的一套纯 C 抽象接口（如 `rmw_publish`）。

## Q4：ROS 2 到 DDS 的调用架构和关系是怎样的？
ROS 2 应用并不直接调用 DDS，而是通过 RMW 层进行多态路由。调用栈关系如下：
1. **Application (应用层)：** 开发者写的 C++/Python Node 代码，调用 `publisher->publish()`。
2. **rclcpp / rcl (ROS 客户端逻辑层)：** 对 API 进行对象化封装和底层 C 逻辑处理。
3. **rmw (中间件抽象层)：** ROS 2 的标准对接层。
4. **rmw_adapter (特定 DDS 适配层)：** 比如 `rmw_fastrtps_cpp` 或 `rmw_cyclonedds_cpp`。它负责将 ROS 的指令和消息类型，翻译为特定 DDS 厂商的专有 API。
5. **DDS DCPS 层 API：** 最终暴露给适配层的标准接口（如 `DataWriter::write()`）。
6. **DDS RTPS 层与 Transport 层：** 执行缓存同步并最终通过 UDP/SHM 扔入物理网卡。
![[Pasted image 20261111172812.png]]