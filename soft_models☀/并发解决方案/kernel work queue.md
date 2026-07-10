
_**理解、总结：
1. 目的：榨干CPU性能，尽量避免CPU执行sleep；
2. 对比user thread pool的关键区别：当work_fn中调用线程阻塞操作时候，（_**调度器操作**_ 本质是执行调度，调度器函数中有个hook函数，在该hook中执行了阻塞状态的记录到线程池管理模块 EXE: nr_running--；然后此时若work list中有未处理的，就会创建新的线程）,nr_running非0代表有worker正在运行，占用CPU，而不是全挂起，此时再创建新thread没有意义，work直接入队列排队；当nr_running=0，说明线程都挂起，CPU可用，此时就会根据线程是否达到限制决定是否创建新thread；
3. 总体模型：
4. **_
# todo: 对比thread；参考新response
---