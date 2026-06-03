
**界限划分：** 实际工程中，**控制面（Control Plane）**（如开关、波特率设置、状态读取）通常用 `sysfs`；而**数据面（Data Plane）**（如读取摄像头视频流、向硬盘写大量数据）则会通过注册字符设备/块设备，在 `/dev` 下生成节点，使用标准的 `read/write/ioctl/mmap` 来操作。


## 🧡不依赖device框架实现的kobject的创建、属性节点创建及user读写流程

这是一个非常棒的学习路径！将理论理解落地为最小可行性代码（MVP），然后再抽象成系统笔记，是掌握 Linux 内核机制最高效的方法。

下面我将为你提供一个开箱即用的最小内核模块 Demo，并附上用户态交互命令；随后，我会整合你的理解与这份代码，为你梳理一份关于“内核对象实现本质”的深度笔记。

### 一、 最小 Demo：内核对象与属性读写

这是一个完整的、可编译的内核模块代码（`kobj_demo.c`）。它会在 `/sys/kernel/` 下创建一个名为 `demo_kobj` 的目录，并在其中创建一个名为 `my_value` 的属性文件。

#### 1. 内核模块代码 (`kobj_demo.c`)

C

```c
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/string.h>

// 定义一个内核变量，用于与用户态交互
static int my_internal_value = 0;
// 指向我们创建的内核对象的指针
static struct kobject *demo_kobj;

/* * 对应用户态的读操作 (如 cat)
 * sysfs_emit 是内核推荐的现代用法，比 sprintf 更安全
 */
static ssize_t my_value_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    pr_info("[Demo] User is reading the value.\n");
    return sysfs_emit(buf, "%d\n", my_internal_value);
}

/* * 对应用户态的写操作 (如 echo)
 */
static ssize_t my_value_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret;
    
    // 将用户传入的字符串转换为整数
    ret = kstrtoint(buf, 10, &my_internal_value);
    if (ret < 0)
        return ret;

    pr_info("[Demo] User updated the value to: %d\n", my_internal_value);
    return count;
}

// 宏定义：绑定属性名称(my_value)、权限(0660)、show函数、store函数
static struct kobj_attribute my_value_attribute = 
    __ATTR(my_value, 0660, my_value_show, my_value_store);

static int __init my_demo_init(void)
{
    int error;

    pr_info("[Demo] Module initializing...\n");

    // 1. 在 /sys/kernel/ 下创建目录 demo_kobj
    demo_kobj = kobject_create_and_add("demo_kobj", kernel_kobj);
    if (!demo_kobj)
        return -ENOMEM;

    // 2. 在该目录下创建属性文件 my_value
    error = sysfs_create_file(demo_kobj, &my_value_attribute.attr);
    if (error) {
        pr_err("[Demo] Failed to create sysfs file\n");
        kobject_put(demo_kobj); // 清理引用
        return error;
    }

    return 0;
}

static void __exit my_demo_exit(void)
{
    pr_info("[Demo] Module exiting...\n");
    kobject_put(demo_kobj); // 引用计数减一，内核会自动清理目录和文件
}

module_init(my_demo_init);
module_exit(my_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gemini");
MODULE_DESCRIPTION("A minimal kobject and sysfs demo");
```

#### 2. 用户态 VFS 交互演示 (终端操作)

当你编译并 `insmod kobj_demo.ko` 加载该模块后，用户侧就可以通过标准 VFS 工具与其交互了：

Bash

```shell
# 1. 写入数据（触发底层 my_value_store）
$ sudo sh -c 'echo 42 > /sys/kernel/demo_kobj/my_value'

# 2. 读取数据并打印到标准输出 (std_out)（触发底层 my_value_show）
$ cat /sys/kernel/demo_kobj/my_value
42

# 3. 查看内核打印的 log (dmesg)
$ dmesg | tail -n 2
[Demo] User updated the value to: 42
[Demo] User is reading the value.
```

### 二、 认知回归：你的理解与机制的映射

结合上面的代码，我们将你上一轮总结的 4 点理解进行具象化印证：

1. **VFS 接口映射**：你在终端敲下的 `cat` 和 `echo`，是纯粹的用户态 VFS 系统调用（`read/write`）。内核通过 `sysfs` 这个基于 RAM 的伪文件系统，完美地将对文件的读写操作，拦截并路由到了控制面（Control Plane）。
    
2. **Kobject 的本质**：代码中的 `kobject_create_and_add("demo_kobj", kernel_kobj)` 证明了它是一个**树状层级节点**（挂载在 `kernel_kobj` 下）。同时，退出函数中仅需调用 `kobject_put(demo_kobj)` 就能完成所有清理，证明了它核心的引用计数（生命周期管理）功能。
    
3. **Operations 接口绑定**：`__ATTR` 宏就像一个胶水，把底层的 `my_value_show` 和 `my_value_store` 塞进了这个属性 Node 的操作函数表中。
    
4. **调用链路打通**：`cat` -> VFS `read()` -> `sysfs/kernfs` 拦截 -> 提取对应 `kobject` 的 `attr` -> 触发执行 `my_value_show` -> 通过 `sysfs_emit` 填入 buffer -> 沿着 VFS 原路返回给 `cat` 程序打印到屏幕终端（std_out）。
    

### 三、 核心架构笔记：Linux 内核对象的实现本质

基于你的思考和上述实践，可以形成以下这份架构层面的笔记：

**主题：Linux 内核对象模型（LDM）与 Sysfs 的实现本质**

- **核心理念：面向对象的内核设计**
    
    Linux 虽由 C 语言写成，但通过 `struct kobject` 实现了极致的面向对象设计。`kobject` 类似于面向对象语言中的“基类（Base Class）”，任何想要拥有“层级关系”、“生命周期托管”和“用户态可见性”的内核结构，只需内嵌一个 `kobject` 即可继承这些能力。
    
- **三位一体的架构支撑**
    
    - **骨架（kobject）：** 负责构建内核对象树，通过 `kref`（引用计数）管理对象的生与死，防止野指针崩溃。它对应 `sysfs` 中的**目录**。
        
    - **血肉（kobj_attribute）：** 负责暴露对象的状态与控制接口。它将内核空间的数据格式化，并对应 `sysfs` 中的**文件（节点）**。
        
    - **桥梁（sysfs/kernfs + VFS）：** 负责突破内核态与用户态的隔离屏障。它利用了“一切皆文件”的思想，将硬件设备、驱动状态抽象为虚拟文件系统，让用户态无需编写复杂的 Socket 或 Netlink 通信代码，仅靠标准 I/O 就能直达内核深处。
        
- **工程意义：控制面与数据面的解耦**
    
    在现代驱动开发中，`sysfs + kobject` 构成了设备的“控制面板”（用于启停、参数配置、调试监控），而将高吞吐量的实际数据流留给 `/dev/` 下的字符/块设备驱动。这种各司其职的设计，保证了内核的稳定与高效。
    

现在，代码和底层逻辑都已经闭环了。在你的实际工程或学习计划中，你是打算基于这套机制为某个具体的硬件写一个驱动，还是在做内核源码的纯理论分析？