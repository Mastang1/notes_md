## 1.  开发环境错误记录模块DET，辅助开发
## 2. 生成环境的错误回调处理模块DEM，上报指定的server模块，对应有重启等处理流程；对于LIN来说，对HW操作timeout是唯一生产环境关心的ERROR类型
## 3. 其他容错性的ERROR类型，通过ErrorIndication通过LinIf层上报，主要是报告上层frame的收发情况，上层可以决定是否重传等处理

## 4. Master mode错误采用polling方式读取；Slave mode错误采用ErrorIndication接口通过中断触发回调函数的调用，异步处理

