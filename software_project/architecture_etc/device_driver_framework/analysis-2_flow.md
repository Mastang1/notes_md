_**个人理解:
0.通过侵入式链表，将dev variable加入到全局的global_list中；
 1 第一层：固定接口，通过name获取设备指针，进而实现多态；总体就是通过name找到一个结构体变量dd；变量中有初始化时候的一个ops，
包含定义的一系列接口；TOP有几个固定的接口，接口中通过dd（open动态化了dd）和dd的ops，实现了多态；
2 只有第一层的问题：该多态只是又mr_dev 扩展到了mr_pin/mr_serial等，跟driver没有关系呢
3 mr_pin等具体设备的接口中，通过mr_dev指针，调用了注册时候赋值的mr_drv,这货包含了执行体和data两个指针，相当于实现了二次扩展
4 最终的效果就是：不同的设备OPS实现了逻辑服务；然后可以调用drv->ops实现底层硬件的控制
5 找时间反推一下：正常顶层的函数是 逻辑调用底层HAL API，如何通过多态实现该结果呢？**_

## 1. Base flow REGISTER
### 1. 根据实际HAL等，实现driver，进行driver initialization（自动）；
### 2. 调用对应设备实体的```
```C
		xxx_register(struct mr_pin  *pin, const char *path, struct mr_drv *drv)//此处以mr_pin为例
```
### 3. 传入该设备框架中定义的 **mr_dev_ops**  和 在驱动开发中定义的 **mr_drv**

### 4. 执行注册/挂在到全局设备链表中；赋值mr_pin中的mr_dev

---

## 2 operations-OPEN()

### 1. 根据name，遍历list，根据对应node的位置偏移获取首地址，但是返回index，named 描述符 fd
### 2. 继续调用 mr_dev在注册时候赋值的mr_dev_ops,这货是统一的接口，但是每个device有不同实现

