
# 0 Relationships(General Objects)
 **1. 协作关系：Notification
	- 场景模式1：单信箱模式(状态覆盖)
		```单个布尔信号，不存储任何状态信息，producter多次发送，会被合并(注意中断也是这种模式)。用于consumer一次性处理所有事件```
	 -场景模式2：带状态存储模式
		 ```producter多次的发送状态都会被记录，consumer会处理多次；信号量和消息队列属于该模式，它俩的区别在于消息队列携带了来自producter的信息```
	

**2. 竞争关系：独占访问
	**独占原因：写同一块ｍｅｍｏｒｙ


## 1. linux kernel wait queue
_**理解笔记：
0.wait queue的本质是事件list，不是可携带消息、实现同步的队列；🌌
1.对比RTOS中的event TCB list，以此为认知锚点展开思路；
2.RTOS kernel中是实现了多个enries方式挂载TCB，所以只可以TCB本体挂载，挂载点是固定的，因而一个TCB同时只可以挂载在一个List；
	但是，Linux kernel的wait queue实现方案不同：a/同样也是一个list；b/挂载方式不同， Linux kernel是通过alloc的entry（include TCB info）挂载，这就可以挂载到多个lists；
3.当一个task挂载到多个list，任意list对应的event就绪，都会唤醒task；所以为了确定是哪个event，需要遍历 描述符列表，select的实现机制；**_

