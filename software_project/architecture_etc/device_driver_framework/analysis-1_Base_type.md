## 1. What
So called Device Driver Framework, 氛围至少3个层次
### Device Management
本质是提供了一个通过基类+派生子类方式形成一个侵入式链表NODE；
该NODE在初始化时候通过按照顺序存储在同名section的方式被Initialization Function 发现并执行初始化注册操作，也就是插入到全局的侵入式链表中；

### Device Model
Create some Device models，which is inherits from mr_dev
_**mr_dev_serial**_

### Driver Adaption(What to do)
Implement the operations structure :
_**struct ops ... **_

## 2. 结构体梳理
### 1. 基本mr_dev:
 ```c
/**
 * @brief Device structure.
 */

struct mr_dev
{
    uint32_t magic;                                                 /**< Magic number */
#ifndef MR_CFG_DEV_NAME_LEN
#define MR_CFG_DEV_NAME_LEN             (8)
#endif /* MR_CFG_DEV_NAME_LEN */

    char name[MR_CFG_DEV_NAME_LEN];                                 /**< Name */
    uint32_t type;                                                  /**< Type */
    int flags;                                                      /**< Flags */
    void *parent;                                                   /**< Parent device */
    struct mr_list list;                                            /**< Same level device list */
    struct mr_list clist;                                           /**< Child device list */
    size_t ref_count;                                               /**< Reference count */

#ifdef MR_USING_RDWR_CTL
    volatile uint32_t lock;                                         /**< Lock flags */
#endif /* MR_USING_RDWR_CTL */

    int sync;                                                       /**< Sync flag */
    int position;                                                   /**< Position */

    struct mr_list rd_call_list;                                    /**< Read callback list */
    struct mr_list wr_call_list;                                    /**< Write callback list */

    const struct mr_dev_ops *ops;                                   /**< Operations */
    const struct mr_drv *drv;                                       /**< Driver */
};
 ```

### 2.  mr_dev_ops

```c
/**
 * @brief Device operations structure.
 */
struct mr_dev_ops
{
    int (*open)(struct mr_dev *dev);
    int (*close)(struct mr_dev *dev);
    ssize_t (*read)(struct mr_dev *dev, void *buf, size_t count);
    ssize_t (*write)(struct mr_dev *dev, const void *buf, size_t count);
    int (*ioctl)(struct mr_dev *dev, int cmd, void *args);
    ssize_t (*isr)(struct mr_dev *dev, int event, void *args);
};
```

### 3. mr_drv
```c
/**
 * @brief Driver structure.
 */
struct mr_drv
{
    void *ops;                                                      /**< Operations */
    void *data;                                                     /**< Data */
};

```

### 4. mr_dev_desc
```c
/**
 * @brief Device descriptor structure.
 */
struct mr_dev_desc
{
    struct mr_dev *dev;                                             /**< Device */
    int flags;                                                      /**< Open flags */
    int position;                                                   /**< Current position */
    struct mr_dev_call rd_call;                                     /**< Read callback */
    struct mr_dev_call wr_call;                                     /**< Write callback */
};
```

