```c
struct sysfs_ops {

    ssize_t (*show)(struct kobject *, struct attribute *, char *);

    ssize_t (*store)(struct kobject *, struct attribute *, const char *, size_t);

};

//ops中是指针，如下是在属性中的具体实现
struct kobj_attribute {

    struct attribute attr;

    ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr,

            char *buf);

    ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr,

             const char *buf, size_t count);

};
全系统场景ICPS系统测试API开发任务
   - 1. 完成全系统场景工程基于mk文件的IPCS driver source加入makefile方式；
     2. 在mcdOs适配层增加mcdOsDelayMs()接口；
     3. 与系统测试同事沟通
```