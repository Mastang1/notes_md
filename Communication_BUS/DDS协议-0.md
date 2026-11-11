>`**DCPS (Data-Centric Publish-Subscribe)**：它是面向应用层的语义抽象模型，负责在内存中构建“参与者/话题/读写端点”的关系树与 QoS 契约，是彻底屏蔽物理网络细节的纯业务意图表达层。`
`**RTPS (Real-Time Publish-Subscribe)**：它是承载 DCPS 的底层二进制有线协议与状态机引擎，负责通过操作底层的 HistoryCache 与收发 HEARTBEAT/ACKNACK 报文，将本地的内存状态变更精准同步到远端。`
`**SPDP (Simple Participant Discovery Protocol)**：它是底层基于组播的节点心跳发现协议，让底层通过周期性广播（类似 Link 16 的 PPLI）宣告“我是谁及我的基础 IP 端口”，用于盲建全局参与者的基础网络拓扑表。`
`**SEDP (Simple Endpoint Discovery Protocol)**：它是基于 SPDP 拓扑之上的可靠元数据握手协议，负责在节点间互换具体话题的读写能力并裁决 QoS 匹配，最终在底层互建 Proxy 代理对象以打通端到端的数据流直连通道。`

---
### 1. 发布-订阅设计模型：
>从直观理解上，是发布者写内容到topic，订阅者抄走这个message，实际本质是订阅后，直接就定向传输了，但是表面看是这么个逻辑；
>**不是每次write都带着topic，这个writer endpoint就只绑定该topic；

### 2. DDS分层（DCPS  &  RTPS）：
### 3.. 动态发现的两个步骤（SPDP  &  SEDP）：

>**关键结构体： 
>2. todo

>**关键的OBJECTS：
>3. todo

>**协议报文格式：
>4. todo



>writer通过topic实现话题组的加入操作：也就是SEDP断点发现协议。通过单播或者组播获取相同topic的participants+endpoint到自己的发送列表中，此后，这个writer就是这个topic的发布者，直接就发到对方了，不搞什么中间黑板这个意象；
## 动态流程

### 1. 控制面流程（类似入网等网络初始化流程）



### 2. 数据面流程（通信建立后的具体的业务流程）



---