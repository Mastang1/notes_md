_**个人理解：总体就是通过name找到一个结构体变量；变量中有初始化时候d**_

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

