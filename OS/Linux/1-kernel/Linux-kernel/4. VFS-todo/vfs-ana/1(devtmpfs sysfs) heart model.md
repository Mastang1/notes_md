## 1. 我的理解
## 1. 我的理解
 - 1. _**devtmpfs**_:根据[[0(VFS)：老子悟道中...]]中对VFS的理解，devtmpfs就是一个基于该类型实现mount在/dev路径下的文件系统，root为/dev;当逻辑device执行mknod等操作时候，是真的在内存中创建对应的dentry和inode的；
 - 2. _**sysfs(内核对象文件系统)**_：属于另一种伪文件系统，是因为它没有具体再ram中存储实际文件系统的dentry等文件节点树，而是通过内存中结构体对象中嵌入kobject(kernfs_node包含)连接而成的list网络 作为伪文件系统结构的替代实现；我已知的包含object的内存对象有class/bus/device/device_driver等，他们也是真实的内存实体；_**简言之：内核对象包含object**_



## 静态构成


## kernel相关场景
![[Pasted image 20260530001951.png]]