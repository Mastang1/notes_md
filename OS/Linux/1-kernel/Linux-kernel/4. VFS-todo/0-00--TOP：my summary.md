## 0. _**从“一切皆文件”的VFS开始
 - 1.  [VFS初始化]VFS 实现统一化的*Interface*，其他文件系统(sysfs/ext4/devtmpfs等)mount到VFS，是为*Implements*，其他文件系统实在kernel初始化过程中mount到VFS；
 - 2. [设备注册]其他文件系统，可以添加文件node到所在文件系统。如：
	 - *devtmpfs:动态创建逻辑device对象，为其赋值设备号/父设备（物理device）等信息，user-space 守护进程调用mknod发送设备号/name等信息，可以创建设备节点到 “/dev/” 路径；
	 - *sysfs：它是内存结构体对象在文件系统的映像，本质是通过 **对象中的kobject成员(kernfs_node)**_在执行add_device()的子操作中，实现向sysfs中添加node，最终展示在 “/sys/” 路径下；*
 - 3. [Open路由泛化]过程：open(file_path)在user space  -  通过SCI陷入kernel，执行VFS 的sys_open操作，获取inode  -  查找空FD（file指针数组的空索引），动态分配/创建 file，根据inode->i_mode(模式掩码)，进入到不同的文件系统implement的open流程，之心file的赋值，对应底层的operations指针赋值到file->ops,返回FD到user space；
	 - 用户态 open("/dev/xxx") 进入内核后，VFS 做路径解析，找到对应 inode。字符设备 inode 中包含设备号 i_rdev。字符设备文件初始的 i_fop 通常是 def_chr_fops，进入 chrdev_open() 后，根据设备号在 cdev_map 中查找对应 struct cdev，拿到驱动注册的 file_operations，替换到 file->f_op，然后调用驱动自己的 open()。



-2.  OPEN的过程理解。
  -A> user-open(file-path), SCI，文件系统通过路径获取文件inode(索引节点)；
   -B.读取inode中的文件信息（todo）获取设备号信息，根据master device number查找cdev_map获取用户定义的cdev静态变量的指针；并动态创建struct file结构体对象，核心赋值 path、inode=通过VFS获取的inode、f_op=底层cdev的元素operations；，其他元素暂时不理会；
   -C.调用driver的open指针函数实现，核心目的是传入一个私有指针，传入user的私有数据，避免open、read、write等接口调用全局或者静态变量；
   -D.加入该file指针到数组中，返回给user空间一个数组元素下标FD；
3.read/write过程。
A> userspace携带入参FD、SCI、VFS模块中通过FD获取file指针，通过file中的f_op执行driver的函数体调用
---

## 1. 问题由来
 - 1. device_add()：宏观结果——/sys/class/driver_name  &  /dev/driver_name；分析两个过程细节
 - 2. sys_open过程，VFS 就是靠 inode->i_mode（文件类型掩码）来实现终极分发的
 - 3. `kobject_add()`实现各个对象的内核文件系统类型展示

## 2. VFS 分析

