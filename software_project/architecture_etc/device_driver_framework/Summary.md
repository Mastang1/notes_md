## 1. What
So called Device Driver Framework, 氛围至少3个层次
### Device Management
本质是提供了一个通过基类+派生子类方式形成一个侵入式链表NODE；
该NODE在初始化时候通过按照顺序存储在同名section的方式被Initialization Function 发现并执行初始化注册操作，也就是插入到全局的侵入式链表中；

### Device Model
Create some Device models，which is inherits from mr_dev
_**mr_dev_serial**_

### Driver Adaption(What to do)
Implement the operations structure :
_**struct ops ... **_

