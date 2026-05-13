### 1. 片段：子系统会向 LDM 申请创建一个分类（Class，显示在 `/sys/class/` 下），同时为了能和应用层通信，它会向系统注册一个或多个 `cdev` 或利用 Network Drivers 机制。

