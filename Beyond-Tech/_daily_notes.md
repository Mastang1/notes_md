#### 1. Vxworks OS 中多核心共享内存，可通过绑定core的方式保障任务的优先级调度；同时，多核也可以采用独立OS，然后通过shared memory方式实现多核通信；
#### 2. 数据收发处理流程中的一些牛逼的概念（驱动/固件/通信）：按照接收中断到来流程开始：
 - 硬件FPGA基带处理，DMA传输数据到ring buffer (一般是pcie总线连接，通过dma传输的，==具体要查)==
 - 接收中断，中断中发送信号量到parser thread
 
 - parser thread loop解析header，然后push到接收process thread（中断底半部）**环形描述符队列**（Descriptor Ring / Scatter-Gather List）
- **解析任务层 (Parser Task)：** 运行于高优先级实时任务中，从接收队列/描述符环中获取数据块，进行帧同步、CRC 校验、解包、ID过滤与 Payload 提取。

- **处理任务层 (Processing Task)：** 将解析后的有效数据（Message/Event）通过无锁环形队列（Lock-free SPSC/MPSC Queue）分发给具体的业务逻辑或控制算法任务。
- 问题：处理不过来导致 《延迟累积》？解决：Process Task校验时间戳，超时任务直接忽略；（但是丢弃之后呢？）



#### 3. 其他
 1. 使用静态内存池，具体查查实现，避免动态内存造成碎片；
 2. 
