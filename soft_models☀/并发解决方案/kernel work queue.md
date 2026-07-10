
_**理解、总结：
1. 目的：榨干CPU性能，尽量避免CPU执行sleep；
2. 对比user thread pool的关键区别：当work_fn中调用线程阻塞操作时候，（本质是执行调度，调度器函数中有个hook函数，在该hook中执行了阻塞状态的记录到线程池管理模块 EXE: nr_running--；）,nr_running代表
3. 总体模型：
4. **_

---