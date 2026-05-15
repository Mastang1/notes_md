

_**个人理解笔记：
1.初步的认知锚点：我一开始认为是：在kernel中的poll执行体中，用了 事件标志组 (本质：可以挂载线程结构指针的 侵入型list)，当所有files都没有data时候被阻塞；
2.实际：底层用的是各个file(including devices driver)的私有等待队列，poll执行体会遍历调用所有file的poll函数，no data则 thread 记录file info、添加thread info 到队列；
3.全无，调用调度器，自己挂载wait queue；
4.有数据，挂载到 ready，并执行clean操作；然后 user代码可以调用后续read()**_
