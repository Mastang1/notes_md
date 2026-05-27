## 1. 个人初始总结
 - 1. 基于面向对象的实现，底层的类型(结构体类型)中 都包含kobject，实现ram中的拓补关系；其中kobj_type *ktype为之添加属性信息：sysfs_ops(读写指针)、等；
 - 2. 其中，kernfs_node * 类型的sd:指向sysfs的路径入口，也就是sysfs一个节点；在kobject被创建中，实现 文件路径创建及双向关联；
 - 2.内存实体（object）和 kernfs_node 的关系被形象理解为：实体和展示窗口

```c
//Kobject
struct kobject {

    const char      *name;

    struct list_head    entry;

    struct kobject      *parent;

    struct kset     *kset;

    const struct kobj_type  *ktype;

    struct kernfs_node  *sd; /* sysfs directory entry */

    struct kref     kref;

  

    unsigned int state_initialized:1;

    unsigned int state_in_sysfs:1;

    unsigned int state_add_uevent_sent:1;

    unsigned int state_remove_uevent_sent:1;

    unsigned int uevent_suppress:1;

  

#ifdef CONFIG_DEBUG_KOBJECT_RELEASE

    struct delayed_work release;

#endif

};
```