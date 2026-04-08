# 1. 采用OS原生 Event Flag Group 事件标志组实现多通知OR方式读取、判断实现条件分支
场景： 当work_thread 需要处于block status，wait forever for 通知；可以通过事件标志组来实现退出和业务逻辑；

# 2. 当采用OS原生Queue时候，可以采用构造特殊command header的 **Poison Pill 毒丸**实现分支，若是有优先级就更好，TCF_SERVER中是这种实现方式 *玻以-zn 皮欧*

# 3. 基于OS 原生Event Group + 自定义Queue实现多条件处理，更加优雅

