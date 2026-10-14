1. static memory pool management
2. add dma bd ring and communication flow
3. cache issue
4. This SDK is 跨平台的 posix接口兼容的功能块
5. push queue is just 方便Rx 查找:分几个步骤：从pool中pop合适的bd   >  push该bd到send queue  > 接收端：从接收队列（发送）pop该bd   > user 执行push bd到 pool bd ring，或者根据需求patch处理；
6. 