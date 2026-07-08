## 1. linux kernel wait queue
_**理解笔记：
0.wait queue的本质是事件list，不是可携带消息、实现同步的队列；🌌
1.对比RTOS中的event TCB list，以此为认知锚点展开思路；
2.RTOS kernel中是实现了多个enries方式挂载TCB，所以只可以TCB本体挂载，挂载点是固定的，因而一个TCB同时只可以挂载在一个List；
	但是，Linux kernel的wait queue实现方案不同：a/同样也是一个list；b/挂载方式不同， Linux kernel是通过alloc的entry（include TCB info）挂载，这就可以挂载到多个lists；
3.当一个task挂载到多个list，任意list对应的event就绪，都会唤醒task；所以为了确定是哪个event，需要遍历 描述符列表，select的实现机制；**_