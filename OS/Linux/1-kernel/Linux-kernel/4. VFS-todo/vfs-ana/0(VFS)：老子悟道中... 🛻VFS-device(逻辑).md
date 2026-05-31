## 1. VFS 是什么
是一个 _**实现沙漏模型**_ 的内核模块,能通过通用的 _**open(file_path)**_ 接口，实现不同类型的file system的延迟绑定，最终实现file对象f_ops的路由赋值，实现 _**泛化**_ ；

## 2. VFS如何实现沙漏模型的（基础结构）
 - 1. 首先VFS实现全局静态管理结构，定义了 图纸集 和 实体集 两个静态链表。图纸集list中有不同类型的文件系统对应的mount函数(建楼方法)，实体集list中是基于图纸实现的不同的 所谓的超级块(以挂载根路径为顶点的一个实体)；
 - 2. 超级块 由 多个inode组合而成，inode在动态添加文件节点时候创建，inode中有个原始的i_fop操作函数组指针，应该是执行节点创建时候直接初始化了一个default，可实现不同的operations；

## 3. VFS进行图纸注册/mount一个超级块
 - reference——[[0(VFS)：VFS添加type mount指定文件系统]]
 - 执行某类型图纸add list；
 - 当user space触发调用sys_mount(type)时候，根据type遍历图纸集，调用图纸mount，创建超级块，带了初始dentry，及inode；

## 4. VFS 的inode创建 mknod `mknod("/dev/ipc-shm", c, dev_id)`
 - i_fop由来：todo


![[Pasted image 20260529000051.png]]