
## 1. 问题由来
 - 1. device_add()：宏观结果——/sys/class/driver_name  &  /dev/driver_name
 - 2. sys_open过程的基于 inode 的imode，实现不同类型的抽象文件系统的泛化，如 设备/sysfs/ext4等

## 2. VFS 分析

