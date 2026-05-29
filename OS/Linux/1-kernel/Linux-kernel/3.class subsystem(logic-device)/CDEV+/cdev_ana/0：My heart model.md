## 1. cdev 创建过程中的边界划分
 - 1. cdev的本质是一个代码组件，实现了核心data及operations打包为struct cdev格式的面向对象的设计。_**对上**_，为了响应LINUX的VFS的设计灵魂，提供了对上承接devtmpfs的 逻辑struct device 对象(including dev_t 设备号)作为接口纽带；_**对下**_，在struct cdev中提供了 `const struct file_operations *ops;`, 方便driver开发者进行适配；
 - 2.  _**alloc_chrdev_region**_:调用设备号管理模块，申请一个主设备号及写入次设备号的范围，主要目的



## 2.  platform-device的启动流程（open *.ko方式）
driver.ko作为文件被insmod或者userspace执行open

