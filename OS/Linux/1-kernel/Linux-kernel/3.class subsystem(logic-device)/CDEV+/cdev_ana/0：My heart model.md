## 1. cdev 创建过程中的边界划分
 - cdev的本质是一个代码组件，实现了核心data及operations打包为struct cdev格式的面向对象的设计。_**对上**_，为了响应LINUX的VFS的设计灵魂，提供了对上承接devtmpfs的 逻辑struct device 对象(including dev_t 设备号)作为接口纽带；_**对下**_，在struct cdev中提供了 `const struct file_operations *ops;`, 方便driver开发者进行适配；
---
#### 属于driver范围
 - 1.  _**alloc_chrdev_region**_:调用设备号管理模块，申请一个主设备号及写入次设备号的范围，主要目的为保证设备号的唯一性；
 - 2. _**void cdev_init(struct cdev *cdev, const struct file_operations *fops)**_, 本质是结构体赋值；

---
#### 属于CDEV范围——插入静态结构(带node)到 cdev管理的一个map中
 - 3. _**static int cdev_add(const char *name, int cdev_id, int max_state, __maybe_unused void *arg)**_, 本质是把配对的(设备号，cdev对象)插入到一个管理链式结构中，为延迟绑定做准备

---
#### 属于FS范围——设备号信息插入到sysfs及devtmpfs两个文件系统(VFS管理)
 - 4. _**struct class *class_create(const char *name)**_, 为内核对象文件系统搞一个class文件夹 dentry，老子现在不知道有鸡毛用，查属性用的吧
 - 5. _**struct device *device_create(const struct class *class, struct device *parent,
                 dev_t devt, void *drvdata, const char *fmt, ...)**_, 目前已知的核心有俩，在sysfs创建dentery(/sys/class/my_first_driver)，在通过user space执行mknod在devtmpfs挂载dentry(/dev/my_first_driver),其中mknod是核心；

补充
```c
# hello


```


---
## 2. Open到最终调用driver operations的open指针流程
 - VFS获取到inode，inode在mknode时候赋值了一个i_ops；先调用i_ops->open操作，跟cdev范围打交道，查map，获取到cdev对象，进行赋值，绑定
 - 
## 3. 基于dev的子系统的二次路由、绑定


## 4.  platform-device的启动流程（open *.ko方式）
driver.ko作为文件被insmod或者userspace执行open

