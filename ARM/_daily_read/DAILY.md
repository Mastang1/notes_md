## 1. cortexM 启动过程
>依赖机制说明：CPU 被设计为从地址4取指令执行(reset_handler 地址)，0 地址放的是 栈顶指针（地址）；然后，CPU执行指令，并执行栈操作；
>也就是，上来执行的是定义的 reset_handler 函数

>所以，代码要想办法把reset_handler放到该地址；