# DRM/HDMI 显示驱动子系统架构笔记：基于“主字符设备一级路由 + 子设备二级路由”心智模型

## 1. 一级路由：DRM 主字符设备与 VFS 接口层

在 VFS 视角的字符设备框架中，DRM 表现为一个主字符设备（Master Character Device），向用户态暴露统一的设备节点（如 `/dev/dri/card0`），主设备号为 226 (`DRM_MAJOR`)。

  

```
/dev/dri/card0 (cdev)
    │
    └── f_op -> drm_fops
                  │
                  ├── .open           = drm_open
                  ├── .unlocked_ioctl = drm_ioctl  ───► [一级路由表: drm_ioctls[]]
                  ├── .mmap           = drm_gem_mmap
                  └── .read / .poll   = drm_read / drm_poll
```

### 1.1 VFS 到 DRM 核心的一级分发机制

- **字符设备绑定**：底层驱动在组件聚合绑定（Component Bind）完成后，调用 `drm_dev_register()`，内部通过 `cdev_add()` 将 `drm_fops` 注入 VFS。
    
      
    
- **一级路由表 (`drm_ioctls`)**：当用户态（如 `libdrm`）发起 `ioctl(fd, DRM_IOCTL_..., arg)` 时，VFS 调用 `drm_ioctl()`。`drm_ioctl()` 提取命令码中的 NR（Number），在静态分发表 `drm_ioctls[]`（定义于 `drivers/gpu/drm/drm_ioctl.c`）中查表，匹配对应的通用 handlers（如 `drm_mode_atomic_ioctl`）或驱动私有 IOCTL handlers。
    
      
    

## 2. 子设备统一抽象模型：`struct drm_mode_object` 与 IDR 树

为了对主字符设备控制下的多个显示子设备（Plane, CRTC, Encoder, Bridge, Connector）进行统一管理，DRM 内核框架采用了类似于面向对象“基类”的设计思想：**所有 KMS 子设备及属性统一定义为 DRM Mode Object**。

  

C

```
// drivers/gpu/drm/drm_mode_object.c
struct drm_mode_object {
    uint32_t id;                  // 全局唯一的 32 位对象 ID (由 IDR 分配)
    uint32_t type;                // 对象类型 (Plane/CRTC/Encoder/Connector/Property)
    struct drm_object_properties *properties; // 关联的属性表
    struct kref refcount;         // 引用计数
    struct drm_device *dev;       // 所属的 DRM 设备句柄
};
```

### 2.1 子设备硬件实体与对象映射表

|**硬件/功能实体**|**子设备对象结构体**|**嵌套的基类**|**核心职责**|
|---|---|---|---|
|**图层/混合器**|`struct drm_plane`|`.base` (`drm_mode_object`)|管理 DMA 图像缓冲区抓取、缩放、Alpha 混合。|
|**显示控制器 Timing**|`struct drm_crtc`|`.base` (`drm_mode_object`)|产生 HSYNC/VSYNC 时序，扫描输出像素流。|
|**信号编码器**|`struct drm_encoder`|`.base` (`drm_mode_object`)|将 CRTC 像素流转换为 HDMI/DP 等具体协议信号。|
|**HDMI 控制器 IP**|`struct drm_bridge`|_非 Mode Object_ (由 Encoder 串接)|封装 SoC 内置或外挂的 DW-HDMI 等 Bridge 芯片逻辑。|
|**物理接口与 Monitor**|`struct drm_connector`|`.base` (`drm_mode_object`)|管理物理连接状态、HPD 中断与 DDC/I2C 读取 EDID。|

### 2.2 子设备 IDR 全局索引树

在 `struct drm_device` 中，维护了一个基于 Radix Tree 的 **IDR（Integer ID Management）机制**：`dev->mode_config.object_idr`。

  

- 当各个子设备（如 HDMI Connector、VOP CRTC）初始化注册时，调用 `drm_mode_object_add()`，系统会自动为其分配一个全局唯一的 32 位整型 `ID`，并挂载到 `object_idr` 树上。
    
      
    
- **这构成了二级路由的寻址基础**。
    
      
    

## 3. 二级路由：基于 Object ID 的子设备精准派发

当控制命令（如 `DRM_IOCTL_MODE_ATOMIC` 或 `DRM_IOCTL_MODE_SETCRTC`）穿透一级路由后，命令结构体中会携带有明确的目标子设备 `obj_id`。DRM 核心库通过 `obj_id` 进行**二级路由派发**。

  

```
[User Space] ioctl(DRM_IOCTL_MODE_ATOMIC, &atomic_cmd)
     │
     ▼ (一级路由)
[drm_ioctl] ──► [drm_mode_atomic_ioctl]
                     │
                     ▼ (提取 command 中的 obj_id)
             [drm_mode_object_find(dev, file_priv, obj_id, type)]
                     │
                     ├─ Search in dev->mode_config.object_idr
                     │
                     ▼ (获取结构体指针，二级路由派发)
         ┌───────────┼───────────┐
         ▼           ▼           ▼
    [drm_plane] [drm_crtc] [drm_connector]
         │           │           │
         └───────────┼───────────┘
                     ▼
         [atomic_check / atomic_flush] ──► 真正写入 SoC 硬件 IP 寄存器
```

### 3.1 二级路由查找核心逻辑 (`drm_mode_object_find`)

以 Atomic Commit 为例，用户态向内核传递一个包含多个（`obj_id`, `property_id`, `value`）元组的数组。内核处理流程如下：

  

1. **IDR 树查找**：`drm_mode_object_find()` 拿着 `obj_id` 检索 `dev->mode_config.object_idr` 树，快速定位到对应的 `struct drm_mode_object *obj`。
    
      
    
2. **类型断言与转换**：根据 `obj->type` 利用 `container_of()` 将通用对象指针转换为具体的 `struct drm_crtc`、`struct drm_plane` 或 `struct drm_connector` 指针。
    
      
    
3. **属性状态集更新**：根据类型分配至相应的子系统状态构建函数：
    
      
    - Plane 对象路由至 `drm_atomic_plane_set_property()`
        
          
        
    - CRTC 对象路由至 `drm_atomic_crtc_set_property()`
        
          
        
    - Connector 对象路由至 `drm_atomic_connector_set_property()`
        
          
        
4. **硬件应用 (Hardware Commit)**：收集好全局 State 后，调用驱动注册的 `drm_mode_config_helper_funcs->atomic_commit_tail()`，最终驱动操作底层 IP 寄存器生效。
    
      
    

## 4. 分布式硬件 IP 的组件聚合 (Component Framework)

由于 HDMI 显示系统涉及多个独立硬件 IP（如 VOP 控制器、DW-HDMI 控制器、I2C/DDC 控制器、PHY 控制器），每个 IP 在 DTS 里都是独立的 `platform_device`。内核使用 Component 框架将这些分散的子设备“打包”为统一的主字符设备。

  

```
                    ┌───────────────────────────────────────────────┐
                    │               Master Component                │
                    │   (rockchip_drm_drv / display-subsystem)      │
                    └───────────────────────┬───────────────────────┘
                                            │
               component_master_add_with_match(&aggregate_driver)
                                            │ (等待所有 Component Probe 完毕)
                                            ▼
┌───────────────────────────────────────────────────────────────────────────────────────┐
│                               component_bind_all()                                    │
├───────────────────────────┬───────────────────────────┬───────────────────────────────┤
│ Component 1 (VOP Driver)  │ Component 2 (HDMI Driver) │ Component 3 (HDMI PHY Driver) │
│ - init drm_plane         │ - init drm_encoder        │ - init PHY HW                 │
│ - init drm_crtc           │ - attach drm_bridge       │                               │
│                           │ - init drm_connector      │                               │
└───────────────────────────┴───────────────────────────┴───────────────────────────────┘
                                            │
                                            ▼
                                  drm_dev_register()
                                            │
                                            ▼
                             创建 /dev/dri/card0 (暴露字符设备)
```

1. **子设备 Probe**：VOP、HDMI 等独立驱动各自在 `platform_driver.probe` 中调用 `component_add()` 注册自己。
    
      
    
2. **Master 匹配**：Master 节点构建匹配表（Match Array）。当所有定义的子 IP 全部加载完成，触发 Master 驱动的 `bind` 函数。
    
      
    
3. **主设备构建与实例化**：Master 驱动分配 `struct drm_device`，调用 `component_bind_all()` 唤醒各个子 IP 的 `bind` 回调函数。各个子 IP 驱动在回调中完成 `drm_crtc_init_with_planes()`、`drm_bridge_attach()`、`drm_connector_init_with_ddc()`，将自己挂入主设备 `object_idr` 树。
    
      
    
4. **字符设备注册**：最终由 Master 驱动调用 `drm_dev_register()`，向 VFS 创建 `/dev/dri/card0` 节点。
    
      
    

## 5. 源码校验与事实核对报告

为了确保上述心智模型与内核代码事实一致，针对 Linux Kernel v6.1+ 核心源码进行逐项校验对比：

  

### 校验点 1：字符设备注册与一级路由分发表

- **源码位置**：`drivers/gpu/drm/drm_fops.c` & `drivers/gpu/drm/drm_ioctl.c`
    
      
    
- **代码事实**：
    
    `drm_fops` 定义了统一的字符设备文件操作集合：
    
      
    
    C
    
    ```
    const struct file_operations drm_fops = {
        .owner = THIS_MODULE,
        .open = drm_open,
        .release = drm_release,
        .unlocked_ioctl = drm_ioctl,
        .mmap = drm_gem_mmap,
        .poll = drm_poll,
        .read = drm_read,
    };
    ```
    
    在 `drm_ioctl()` 函数内部，提取 `cmd` 后直接查表 `drm_ioctls`：
    
      
    
    C
    
    ```
    nr = _IOC_NR(cmd);
    if (nr < DRM_COMMAND_BASE || nr >= DRM_COMMAND_END)
        ioctl = &drm_ioctls[nr]; // 查一级路由表
    ```
    
- **结论**：**符合代码事实**。字符设备入口与一级路由映射逻辑完全成立。
    
      
    

### 校验点 2：子设备 `drm_mode_object` IDR 注册与索引查找

- **源码位置**：`drivers/gpu/drm/drm_mode_object.c`
    
      
    
- **代码事实**：
    
    新对象注册函数 `drm_mode_object_add()`：
    
      
    
    C
    
    ```
    int drm_mode_object_add(struct drm_device *dev, struct drm_mode_object *obj, uint32_t obj_type)
    {
        // ...
        ret = idr_alloc(&dev->mode_config.object_idr, register_obj ? obj : NULL, 1, 0, GFP_KERNEL);
        if (ret < 0) return ret;
        obj->id = ret; // 分配全局唯一 ID
        obj->type = obj_type;
    }
    ```
    
    二级路由根据 ID 提取对象函数 `drm_mode_object_find()`：
    
      
    
    C
    
    ```
    struct drm_mode_object *drm_mode_object_find(struct drm_device *dev, struct drm_file *file_priv, uint32_t id, uint32_t type)
    {
        // 从 IDR 查找指针
        obj = idr_find(&dev->mode_config.object_idr, id);
        // 校验 obj 匹配类型 ...
        return obj;
    }
    ```
    
- **结论**：**符合代码事实**。IDR 树实现了统一对象的全局 ID 分配与快速查找。
    
      
    

### 校验点 3：Atomic Commit 的二级路由分发逻辑

- **源码位置**：`drivers/gpu/drm/drm_atomic_uapi.c`
    
      
    
- **代码事实**：
    
    在 `drm_mode_atomic_ioctl()` 处理循环中：
    
      
    
    C
    
    ```
    for (i = 0; i < arg->count_objs; i++) {
        // 1. 二级路由：根据传递的 obj_id 查找具体的 mode_object
        obj = drm_mode_object_find(dev, file_priv, obj_id, DRM_MODE_OBJECT_ANY);
    
        // 2. 内部根据 obj->type 路由派发给各个子设备的状态设置回调
        switch (obj->type) {
        case DRM_MODE_OBJECT_CONNECTOR:
            ret = drm_atomic_connector_set_property(connector, state, prop, val);
            break;
        case DRM_MODE_OBJECT_CRTC:
            ret = drm_atomic_crtc_set_property(crtc, state, prop, val);
            break;
        case DRM_MODE_OBJECT_PLANE:
            ret = drm_atomic_plane_set_property(plane, state, prop, val);
            break;
        }
    }
    ```
    
- **结论**：**符合代码事实**。Atomic Commit 完全依赖 `obj_id` 作为 key 进行二级路由，精准控制具体的底层 Plane/CRTC/Connector 子设备。