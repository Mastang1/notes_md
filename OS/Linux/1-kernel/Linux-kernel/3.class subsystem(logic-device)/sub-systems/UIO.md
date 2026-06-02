## 1. 个人理解
 - 1.  业务功能：通过UIO子设备延迟绑定实现VFS的 文件接口，核心功能是提供了：基于mmap的user直接内存访问；基于kernel wait_queue实现的中断信号通知到user空间（基于read接口）；
 - 2. VFS接口路由架构：在uio.c这个uio单元中，创建名为uio的cdev的master设备，分配了class、cdev的operations；执行master的open中，进行子设备号的查找、路由，具体参考2.1；
	 - 2.1 子设备注册：(以cdev的add_device为锚点)
	 - 2.2 子设备open：
 - 3.  子设备管理实现(其他子设备管理是否也是相同架构)：
 - 