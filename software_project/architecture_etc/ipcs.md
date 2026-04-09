### app - shm - os - hw
### 执行流：
 - 1 正向调用
 - 2. 中断/thread 执行回调 （前提：在正向调用的Initialization阶段执行CB注册：如app CB、shm CB注册到下层代码组件中）
