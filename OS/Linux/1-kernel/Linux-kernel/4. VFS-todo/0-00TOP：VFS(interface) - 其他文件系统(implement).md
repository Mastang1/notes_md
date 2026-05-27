## 0. _**从“一切皆文件”的VFS开始
 - 1.  


---

## 1. 问题由来
 - 1. device_add()：宏观结果——/sys/class/driver_name  &  /dev/driver_name；分析两个过程细节
 - 2. sys_open过程，VFS 就是靠 inode->i_mode（文件类型掩码）来实现终极分发的
 - 3. `kobject_add()`实现各个对象的内核文件系统类型展示

## 2. VFS 分析

