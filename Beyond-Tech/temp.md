1. static memory pool management
2. add dma bd ring and communication flow
3. cache issue
4. This SDK is 跨平台的 posix接口兼容的功能块
5. push queue is just 方便Rx 查找:分几个步骤：从pool中pop合适的bd   >  push该bd到send queue  > 接收端：从接收队列（发送）pop该bd   > user 执行push bd到 pool bd ring，或者根据需求patch处理；
>1. 发送端的pool 的bd ring，是映射多个保存内存信息的structure，acquire时候执行pop，**类似把一个架子上的一个衣服架取走了（或者说是 桶 是数据容器）**；此时执行遍历，找不到这个pop的 BD；
>2. driver的实现，是用通道的queue保存了该衣服架信息，执行遍历该通道queue就知道哪些通道发送了数据，且可以找到memory，然后再在收端push数据到pool的bd ring；
>3. **Channel queue 初始为空队列；Pool queue初始化后是包含了多个buffer地址信息的满队列；场景流程是：选取一个合适的bd>buffer, 写payload，pop 该BD并push到 channel队列（这俩过程是copy操作）；接收端pop from Channel Queue；然后 user process in callback， 最后push BD到Pool queue，完成闭环；
>4. DMA BD ring，是固定链式bd数组，不执行pop、push操作，由双方执行ownership检查实现数据的归属判断；
>5. ipcs的提供chan'n'ne'l
6. 遵循 架构设计、接口设计、详细设计的流程
7. 内存流转图
8. 理论模型
9. IPC中断底半部的批处理操作