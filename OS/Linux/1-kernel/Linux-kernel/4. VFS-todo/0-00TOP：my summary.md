## 0. _**从“一切皆文件”的VFS开始
 - 1.  [VFS初始化]VFS 实现统一化的*Interface*，其他文件系统(sysfs/ext4/devtmpfs等)mount到VFS，是为*Implements*，其他文件系统实在kernel初始化过程中mount到VFS；
 - 2. [设备注册]其他文件系统，可以添加文件node到所在文件系统。如：
	 - *devtmpfs:user-space 守护进程调用mknod发送设备号/name等信息，可以创建设备节点到 “/dev/” 路径；
	 - *sysfs：它是内存结构体对象在文件系统的映像，本质是通过 **对象中的kobject成员(kernfs_node)**_在执行add_device()的子操作中，实现向sysfs中添加node，最终展示在 “/sys/” 路径下；*
 - 3. [Open路由泛化]


---

## 1. 问题由来
 - 1. device_add()：宏观结果——/sys/class/driver_name  &  /dev/driver_name；分析两个过程细节
 - 2. sys_open过程，VFS 就是靠 inode->i_mode（文件类型掩码）来实现终极分发的
 - 3. `kobject_add()`实现各个对象的内核文件系统类型展示

## 2. VFS 分析

