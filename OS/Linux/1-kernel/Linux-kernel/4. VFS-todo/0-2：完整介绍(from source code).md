

以下是为您定制的《VFS与底层文件系统接口架构及核心场景剖析》演示文稿内容规划。本规划严格基于 Linux Kernel 源码树结构（如 `fs/`、`include/`、`Documentation/`）提取，采用费曼图式映射，并仅保留核心的静态架构与动态序列图设计，绝不附加任何无关信息。

---

### 一、 静态结构：VFS 与具体文件系统（sysfs/devtmpfs/ext4）的接口架构

**核心设计思想**：面向对象中的“接口与实现分离”。VFS 提供统一的纯虚类定义（`file_system_type`, `super_block`, `inode`），而 ext4、sysfs、devtmpfs 作为派生类提供具体实现。

**组件间交互映射（UML 组件图）**：

![[Pasted image 20260528222832.png]]

---
### 二、 核心流程 1：文件系统注册到 VFS 与挂载为 Super Block

**场景说明**：系统启动时，内核子系统将自身的 `file_system_type` 挂入 VFS 链表，随后通过 `sys_mount` 触发底层读取介质并生成 `super_block` 实例。

**注册与挂载序列图**：

```
sequenceDiagram
    autonumber
    participant Kernel as 内核初始化 (Init)
    participant VFS_Reg as VFS 注册表 (fs/filesystems.c)
    participant VFS_Mount as VFS 挂载核心 (fs/namespace.c)
    participant FS_Driver as 底层文件系统 (如 ext4/sysfs)

    Note over Kernel, FS_Driver: 阶段 A: 静态注册营业执照
    Kernel->>VFS_Reg: register_filesystem(&ext4_fs_type)
    VFS_Reg->>VFS_Reg: 将 fs_type 挂入内核全局 file_systems 链表

    Note over Kernel, FS_Driver: 阶段 B: 动态挂载并生成超级块
    Kernel->>VFS_Mount: sys_mount("ext4", "/mnt", "ext4", ...)
    VFS_Mount->>VFS_Reg: get_fs_type("ext4") (查找注册表)
    VFS_Mount->>FS_Driver: fs_type->mount() 或 init_fs_context()

    FS_Driver->>FS_Driver: 探查块设备/分配内存，解析底层数据
    FS_Driver->>FS_Driver: 分配并填充 struct super_block
    FS_Driver->>FS_Driver: 填充根目录 inode 与 dentry
    FS_Driver-->>VFS_Mount: 返回初始化完成的 super_block 指针

    VFS_Mount->>VFS_Mount: vfs_get_tree(): 将新超级块拼接至 VFS 统一目录树的 "/mnt"
```

---

### 三、 核心流程 2：文件系统中重添加 Node（举例 cdev 添加到 /dev 及 sysfs）

**场景说明**：驱动执行 `device_create` 时，内核设备模型（LDM）将兵分两路，分别在 `sysfs` 中利用 `kernfs` 直接生成状态节点，在 `devtmpfs` 中利用用户态守护进程协助生成通信节点。

**双线建档序列图**：

```
sequenceDiagram
    autonumber
    participant Driver as 设备驱动
    participant LDM as 设备模型 (drivers/base/core.c)
    participant Sysfs as Sysfs / Kernfs (fs/sysfs)
    participant Udev as 用户态 (udevd)
    participant Devtmpfs as Devtmpfs 引擎 (fs/devtmpfs)

    Driver->>LDM: device_create(class, devt, "my_cdev")

    Note over LDM, Sysfs: 路线 1: 挂载到 sysfs (纯内核态目录树操作)
    LDM->>Sysfs: kobject_init_and_add() -> sysfs_create_dir_ns()
    Sysfs->>Sysfs: kernfs_create_dir("my_cdev") (生成目录)
    LDM->>Sysfs: sysfs_create_file() -> kernfs_create_file()
    Sysfs->>Sysfs: 在目录中生成 Uevent 等属性文件 (S_ISREG)

    Note over LDM, Devtmpfs: 路线 2: 挂载到 devtmpfs (/dev 下的字符节点)
    LDM->>Udev: kobject_uevent() (通过 Netlink 广播主次设备号 devt)
    Udev->>Devtmpfs: 解析 Uevent，执行 mknod("/dev/my_cdev", S_IFCHR, devt)
    Devtmpfs->>Devtmpfs: 在 /dev 挂载点分配新的 struct inode
    Devtmpfs->>Devtmpfs: inode->i_mode = S_IFCHR (标记为字符设备)
    Devtmpfs->>Devtmpfs: inode->i_rdev = devt (固化设备号)
```

---

### 四、 核心流程 3：SCI 调用后执行 VFS 的 open 操作详细流程

**场景说明**：App 通过系统调用（SCI）发起 `open`，VFS 核心进行路径解析（Namei），提取底层 `inode`。若是字符设备，则触发终极的“多态替换（狸猫换太子）”，通过哈希表溯源获取真正的 `cdev` 操作集。

**VFS Open 穿透底层序列图**：

```
sequenceDiagram
    autonumber
    participant App as 应用程序 (User Space)
    participant SCI as 系统调用层 (fs/open.c)
    participant Namei as 路径解析 (fs/namei.c)
    participant VFS as VFS 核心装配
    participant CdevMap as cdev_map 路由表 (fs/char_dev.c)
    participant Target as 目标驱动 (cdev->ops)

    App->>SCI: open("/dev/my_cdev", O_RDWR)
    SCI->>SCI: do_sys_openat2() -> alloc_empty_file() (分配空 file 结构)

    Note over SCI, Namei: 提取底层实体身份 (inode)
    SCI->>Namei: path_openat("/dev/my_cdev")
    Namei->>Namei: link_path_walk() (查 dcache 或底层 FS)
    Namei-->>SCI: 返回解析成功的 dentry 与对应的 inode

    Note over SCI, VFS: 多态分发：VFS 操作函数挂载
    SCI->>VFS: vfs_open(path, file)
    VFS->>VFS: file->f_op = inode->i_fop (赋予文件系统默认操作，如 def_chr_fops)

    Note over VFS, Target: S_ISCHR 分支的特殊路由：操作集替换
    VFS->>CdevMap: file->f_op->open() -> 触发 chrdev_open(inode, file)
    CdevMap->>CdevMap: 以 inode->i_rdev (设备号) 为 Key 查找 kobj_map
    CdevMap-->>VFS: 返回匹配的真实 struct cdev * 指针

    VFS->>VFS: replace_fops(file, cdev->ops) (狸猫换太子，替换 f_op)
    VFS->>Target: file->f_op->open(inode, file) (直接穿透调用目标驱动 open)
    Target-->>VFS: 驱动硬件初始化完成

    SCI->>SCI: fd_install(fd, file) (将会话装入进程描述符表)
    SCI-->>App: 返回文件描述符 fd
```