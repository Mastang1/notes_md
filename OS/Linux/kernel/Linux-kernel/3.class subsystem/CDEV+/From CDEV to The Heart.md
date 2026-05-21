
## temp 静态图
![[Pasted image 20260521000720.png]]

## CDEV 几个APIs的调用流程 及 操作对象

![[Pasted image 20260521000630.png]]


## mr-library 设备open流程  VS VFS sys_open流程
_**我暂时理解：这只是发现过程
注册过程呢：也就是设备路径  到 设备号的关联过程（match）或者叫做动态生成 TODO**_

![[Pasted image 20260521005010.png]]

![[Pasted image 20260521004532.png]]


## 获取设备号 将设备号和我的cdev结构体首地址存入map table

todo：

在 `cdev_map`（全局哈希字典）里，新建了一个 `probe`（字典条目）节点。

- 把你申请的设备号（`dev_num`）存入了 `probe->dev`。
    
- 把你驱动里的 `my_cdev` 的内存首地址，存入了 `probe->data`。


