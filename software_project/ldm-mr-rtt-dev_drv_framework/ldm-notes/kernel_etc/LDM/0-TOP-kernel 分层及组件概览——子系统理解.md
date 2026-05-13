

##  系统架构
![[SWC RTE LINIF Data Pipeline-2026-05-13-021102.png]]

## 所谓的子系统(属 软件中间件)
_**个人理解笔记：
1.LDM 实现不同bus的设备注册（match）及统一的管理（电源、增删等）
2.CDEV是一个比较标准的class。其功能是实现对上提供下层驱动的raw data的读写；其本质是封装了互斥锁及一些基本逻辑的组件，对上为VFS提供接口；对下提供一个operations函数指表；在LDM的probe函数中实现赋值，这个operations也是对具体driver的解耦依赖接口。这样，底层硬件物理挂载那种bus，或者那种不同类型的地址寄控制器，都不影响cdev；
3.所谓的子系统，属于class，可以类比cdev，认为 是基础的三个IO设备模型的扩展。其功能是提供对上层的结构化数据，接口多为VFS接口，只是读写的数据格式化了；对下也一般提供一个operations，用作底层driver开发的适配接口，实现子系统和底层驱动的解耦；**_


![[SWC RTE LINIF Data Pipeline-2026-05-13-034056.png]]
