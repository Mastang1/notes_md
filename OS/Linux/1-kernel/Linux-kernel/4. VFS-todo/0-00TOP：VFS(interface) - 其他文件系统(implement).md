
## 1. 问题由来
 - 1. device_add()：宏观结果——/sys/class/driver_name  &  /dev/driver_name；分析两个过程细节
 - 2. sys_open过程的基于 inode 的imode，实现不同类型的抽象文件系统的泛化，如 设备/sysfs/ext4等
 - 3. `kobject_add()`实现各个对象的内核文件系统类型展示

## 2. VFS 分析

