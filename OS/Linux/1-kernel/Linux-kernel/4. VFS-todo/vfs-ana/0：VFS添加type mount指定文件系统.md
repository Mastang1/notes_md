## Q:流程一 - 文件系统注册与挂载（如何平地起高楼）
概述：详解内核是如何把 ext4 或 sysfs 这张图纸塞进单向链表，然后又是如何通过 mount 动作，真正在内存中把 super_block 大厦盖起来，并把一楼大门对接进 VFS 树状系统的

## A:
你好！我是 Arch-ldm-rtt-mr。收到你极其明确且严厉的指令！上一回合我确实步子迈得太大，把内容杂糅了。作为架构导师，我完全接受你的纠偏，现在我们**立刻单点爆破“问题 2：流程一 - 文件系统注册与挂载（如何平地起高楼）”**。

我们将严格使用**费曼技巧（图式理论）**，把你熟悉的“单片机链表操作”作为锚点，彻底讲透内核是怎么把“图纸（`file_system_type`）”变成“真实大厦（`super_block`）”，并最终开门营业的。

---

### 费曼隐喻导入：万达广场的图纸与实体

在 Linux 的世界里，VFS（虚拟文件系统）就是国家的“住建部”。

1. **图纸（`file_system_type`）**：这是开发商提交的设计图，比如“ext4 万达广场图纸”或“sysfs 政务中心图纸”。图纸只有一张，存放在住建部的**单向链表**档案柜里。
2. **大厦实体（`super_block`）**：图纸不能直接用来卖东西（读写数据）。当你在终端敲下 `mount` 命令时，施工队根据图纸，真正在内存（和磁盘）上盖起了一座大厦。插上 3 个 ext4 格式的 U 盘，就会生成 **3 座独立的 `super_block` 大厦**。
3. **一楼总大门（`s_root` dentry）**：大厦盖好后，必须有一扇门。这扇门最终会被“焊接”到 VFS 的全局树状路标上（比如挂载点 `/mnt`）。

---

### 一、 静态组件关系：图纸、大厦与大门 (UML 组件图)

我们先看在执行动作之前，这些对象在内存中是如何嵌套的。

![[Pasted image 20260529002416.png]]

---

### 二、 动态流程剖析：平地起高楼的 2 个阶段 (UML 序列图)

这是从系统启动到用户执行 `mount` 命令时的完整动态执行流。

![[Pasted image 20260529002341.png]]

---

### 三、 核心伪代码展示（用 C 语言还原第一性原理）

为了让你彻底吃透，我们把底层的魔法剥开，看看 C 代码层面是如何操作链表和对象的：

#### 1. 阶段 A：图纸塞进单向链表 (注册)

这部分代码极其简单，就是一段经典的单片机级别的**单向链表尾部插入**操作。

```c
// 内核全局指针，指向图纸档案柜的第一个抽屉
static struct file_system_type *file_systems;

int register_filesystem(struct file_system_type * fs)
{
    struct file_system_type ** p;
    // 1. 遍历单向链表
    for (p = &file_systems; *p; p = &(*p)->next) {
        if (strcmp((*p)->name, fs->name) == 0)
            return -EBUSY; // 发现重名图纸，拒绝注册
    }

    // 2. 挂入链表尾部
    fs->next = NULL;
    *p = fs;
    return 0;
}
```

#### 2. 阶段 B：通过 mount 动作建大厦并对接大门

当用户态下发 `mount -t ext4 /dev/sda1 /mnt` 时，VFS 施工队的核心动作如下：

```c
struct dentry *mount_fs(struct file_system_type *type, int flags, const char *name, void *data)
{
    struct dentry *root;
    struct super_block *sb;

    // 1. 核心调用：执行具体文件系统（图纸）里的建楼方法！
    // 比如 ext4 的 mount 方法会在这里被执行，去读磁盘并分配 super_block 和根 dentry
    root = type->mount(type, flags, name, data);

    // 2. 建楼完毕，溯源检查
    sb = root->d_sb; // 从一楼大门反向获取整个大厦的对象指针
    BUG_ON(!sb);     // 确保大厦真的盖好了

    // 3. 完善大厦的属性标签
    sb->s_flags |= SB_BORN; // 标记大厦正式落成

    // 4. 返回一楼大门的钥匙，交由外层的 namespace.c 将其拼接到 "/mnt" 节点
    return root;
}
```

### 架构师课后微总结

到这里，你已经亲眼看着 VFS 宇宙的第一块砖是如何垒起来的了。

- `file_systems` 这个单向链表解决的是**“系统支持哪些格式”**的问题。
- `super_block` 及其伴生的根 `dentry` 解决的是**“这块硬盘/内存现在正在被作为一个独立的大厦使用”**的问题。
- 最后那一下嫁接（Mount），彻底让独立的大厦接入了 Linux 的 `/` 根目录交通网中。
