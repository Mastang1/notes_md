1. static memory pool management
2. add dma bd ring and communication flow
3. cache issue
4. This SDK is 跨平台的 posix接口兼容的功能块
5. push queue is just 方便Rx 查找:分几个步骤：从pool中pop合适的bd   >  push该bd到send queue  > 接收端：从接收队列（发送）pop该bd   > user 执行push bd到 pool bd ring，或者根据需求patch处理；
>1. 发送端的pool 的bd ring，是映射多个保存内存信息的structure，acquire时候执行pop，**类似把一个架子上的一个衣服架取走了（或者说是 桶 是数据容器）**；此时执行遍历，找不到这个pop的 BD；
>2. driver的实现，是用通道的queue保存了该衣服架信息，执行遍历该通道queue就知道哪些通道发送了数据，且可以找到memory，然后再在收端push数据到pool的bd ring；
>3. **Channel queue 初始为空队列；Pool queue初始化后是包含了多个buffer地址信息的满队列；场景流程是：选取一个合适的bd>buffer, 写payload，pop 该BD并push到 channel队列（这俩过程是copy操作）；接收端pop from Channel Queue；然后 user process in callback， 最后push BD到Pool queue，完成闭环；
>4. Channel Queue pop和 callback中执行pool queue的push是两个不同的Events，前者标识通知接收，后者标识_*消费完成；*_
>5. DMA BD ring，是固定链式bd数组，不执行pop、push操作，由双方执行ownership检查实现数据的归属判断；
>6. ipcs的提供channel queue的实现，方便用于多级消息的zero-copy 传递。
>7. 最终模型：ISR 管通知、屏蔽中断，触发通知到底半部；底半部执行数据批处理（也可能加入配额管理）本质是消费，然后归还bd;
>8. ### Copy 与 Zero-Copy 的实际取舍(tdma 的网口报文0 copy传输流程场景处理)

| 数据特征           | 更值得优先测试的方案       | 原因                     |
| -------------- | ---------------- | ---------------------- |
| 64～256 B 小包    | 直接 Copy          | 元数据、引用计数、SG 描述符成本可能占主导 |
| 512 B～2 KB     | 原地替换或 Copy       | 两者都可能很快，需要基准测试         |
| 4 KB～64 KB     | 原地或 SG Zero-Copy | Payload 搬移成本逐渐显著       |
| 大块连续流数据        | SG / Zero-Copy   | 降低 CPU 数据搬移及内存带宽压力     |
| 多路共享相同 Payload | 引用计数 + SG        | 避免每个消费者复制一份            |
6. 遵循 架构设计、接口设计、详细设计的流程
7. 内存流转图
8. 理论模型
9. IPC中断底半部的**批处理操作**（基于**配额**的局部polling操作）
10. tdma 指控中心发送周期：timeout、发送结果返回；
11. 发送指标：端到端延迟；指定时隙配置下的吞吐量；burst发送