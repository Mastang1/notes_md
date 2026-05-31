
## 1. 我的理解 _**(devtmpfs ram中是真实的inode集合;sysfs，kobject(inode的记录卡)/三大list_node(符号链接) )

 - 1. _**devtmpfs**_:根据[[0(VFS)：老子悟道中... 🛻VFS-device(全流程逻辑)]]中对VFS的理解，devtmpfs就是一个基于该类型实现mount在/dev路径下的文件系统，root为/dev;当逻辑device执行mknod等操作时候，是真的在内存中创建对应的dentry和inode的；
 - 2. _**sysfs(内核对象文件系统)**_：属于另一种伪文件系统，是因为它没有具体再ram中存储实际文件系统的dentry等文件节点树，而是通过内存中结构体对象中嵌入kobject(kernfs_node包含)连接而成的list网络 作为伪文件系统结构的替代实现；我已知的包含object的内存对象有class/bus/device/device_driver等，他们也是真实的内存实体；_**简言之：内核对象包含kobject就可以动态转为inode，是个实体inode，不是链接；**_
		但是，通过挂载device->p下的**parent_node/class_node/bus_node**到对应的父设备/类型/总线的链表下，就认为是在对应实体的下一级目录中；[[Pasted image 20260530010332.png]]
		
![[Pasted image 20260530010332.png]]


## 静态构成


## kernel相关场景
![[Pasted image 20260530001951.png]]