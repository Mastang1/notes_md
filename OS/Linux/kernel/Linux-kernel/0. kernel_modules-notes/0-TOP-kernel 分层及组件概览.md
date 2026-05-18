## summary
 - 1. 总体架构，如下 系统架构所示，个人总结的分层视图：user app/libc，posix layers, SCI , VFS, sub-system, LDM(operations), hardware;
 - 2. 多任务、任务通信等OS功能，TODO；
 - 3. 内存管理， TODO；
 - 4. sub-system:一些列的逻辑框架，实现某种功能逻辑、运行状态的封装的功能子模块，对上一般提供operations的标准接口的实现，如在open/read/write等对上接口中实现；对下一般提供了标准的契约，如函数接口，待LDM的driver进行实现（驱动开发工程师实现）；
 - 5. sub-system/class：目前认为每个子系统都可以在probe流程中在class元素中赋值，以标识不同的 设备类型；class本身只是一个封装了list node的扩展节点，最终会挂载到对应类型的list上；
 - 6. 三个 基础IO设备：char、block：可以认为是子系统的基础，一般被子系统调用；功能是实现kernel module 与user space进行交互；
 - 7. LDM：Linux Device Model，我又称其为 bus-device-driver framework。
     _**功能：**_1. 将设备(具体的data parameter)/驱动(具体的功能逻辑)进行分割；2. 按照bus类型进行分类，但提供共同的设备-驱动匹配、probe、电源管理、设备管理(开闭等)的对外API，不同BUS有不同的内部实现；3. 常见的总线类型有： 片上资源 platform bus、实际总线类有 SPI 、IIC 、CAN等；
     _**与子系统的接口：**_在driver的实现中，子系统提供上层实现，并提供具体的接口，用于在probe中执行初始化赋值
##  系统架构
_**注意的点：
 1 注意POSIX 和 libc的两种接口的区别，posix 在libc之后；区别在于libc有缓存，即加入了线程等逻辑层；posix直接最终操作的的是底层的operations，没有缓冲；**_
![[SWC RTE LINIF Data Pipeline-2026-05-13-021102.png]]

## 所谓的子系统(属 软件中间件)
_**个人理解笔记：
1.LDM 实现不同bus的设备注册（match）及统一的管理（电源、增删等）
2.CDEV是一个比较标准的class。其功能是实现对上提供下层驱动的raw data的读写；其本质是封装了互斥锁及一些基本逻辑的组件，对上为VFS提供接口；对下提供一个operations函数指表；在LDM的probe函数中实现赋值，这个operations也是对具体driver的解耦依赖接口。这样，底层硬件物理挂载那种bus，或者那种不同类型的地址寄控制器，都不影响cdev；
3.所谓的子系统，属于class，可以类比cdev，认为 是基础的三个IO设备模型的扩展。其功能是提供对上层的结构化数据，接口多为VFS接口，只是读写的数据格式化了；对下也一般提供一个operations，用作底层driver开发的适配接口，实现子系统和底层驱动的解耦；**_


![[SWC RTE LINIF Data Pipeline-2026-05-13-034056.png]]
