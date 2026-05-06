### app - shm - os - hw
 - 具体架构设计参考 架构设计文档
### 为啥用双环QUEUE
 - 好处：只读对侧memory；只写本册memory
### 描 述
一个QUEUE，包含两个ring（FIFO），
##### push ring（LOCAL）
##### pop ring(REMOTE)
上述俩货偏移相同，在代码中可以方便实现对端操作；
### 动态——扁平流程，发送、接收为两端，非一端
执行发送等场景：push： write push ring， 写BD到local的data[]
执行接收场景：读取对端push ring中的write，更新自己端的read，读取对端的data