### Flow
 - 1. platform_device add到platform bus中，设备树转换或者静态定义实现；
 - 2. 设备插入 或者手动调用driver时候，遍历注册的设备list，执行match(返回匹配的device指针)，调用probe(返回的device)，次probe函数体是driver开发者定义，但是调用、和参数是确定的；
 - 3._**进入到逻辑设备的领域**_
- 