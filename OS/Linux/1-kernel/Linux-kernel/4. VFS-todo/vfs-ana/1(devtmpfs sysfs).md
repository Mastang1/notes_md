## 1. 我的理解
 - 1. _**devtmpfs**_:根据[[0(VFS)：老子悟道中...]]中对VFS的理解，devtmpfs就是一个基于该类型实现mount在/dev路径下的文件系统，root为/dev;当逻辑device执行mknod等操作时候，是真的在内存中创建对应的dentry和inode的；
 - 2. 