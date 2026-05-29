## 一切操作的核心对象就是 device & kobject(kernfs_node)
 - 这两个文件系统的操作核心对象就是device；
 - 通过kobject的kernfs_node成员作为sysfs生成inode的本体；
 - 通过device中的私有的list_node实现不同的文件层级及同级关联；


![[Pasted image 20260530002620.png]]