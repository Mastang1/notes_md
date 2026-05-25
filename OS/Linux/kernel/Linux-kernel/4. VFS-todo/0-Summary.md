_**实现漏斗模式的逻辑理解：
1.user：kernel VFS 这两个映射的调用中的理解。struct file *filp是FD在kernel的一个代理，毕竟不能直接用指针跨空间访问，使用struct file指针的数组的下标很合适。
2.OPEN的过程理解。user-open(file-path), SCI，文件系统通过路径获取文件inode(索引节点),并动态创建struct file结构体对象，赋值：**_