在整个图形与显示栈中，**从 Qt 到硬件屏幕绝非逐层拷贝“一帧全屏像素数据”**，而是采用了“零拷贝（Zero-Copy）”与“描述符传递”架构。

  

物理像素数据仅在 Qt/GPU 渲染时被写入一次内存（RAM/VRAM），后续所有用户态与内核态层级之间，传递的全部是**内存句柄（File Descriptor）、C 语言结构体对象、图层 ID 及坐标元数据**。真正的像素搬运仅由显示控制器（VOP/DC）的 **DMA 硬件引擎**在最终扫屏时完成。

  

## 1. 分层接口实现机制与数据传递对照表

|**层级跨越 (From → To)**|**接口实现机制 (Mechanism)**|**传递的数据结构 / 类 / 对象**|**实际传输的内容 (Payload)**|
|---|---|---|---|
|**1. Qt App $\rightarrow$ QPA 插件**|C++ 虚函数重写 & 回调函数 (C++ VTable)|`QMouseEvent`<br><br>  <br>  <br><br>`QWaylandWindow`<br><br>  <br>  <br><br>`QWaylandEglWindow`|坐标点、按钮状态、重绘区域 `QRegion` 元数据|
|**2. Qt QPA $\rightarrow$ Wayland IPC**|`libwayland-client` API (C 语言函数封装)，向 Unix Socket 写入|Proxy 对象 (`wl_proxy`)<br><br>  <br>  <br><br>`zwp_linux_dmabuf_v1`<br><br>  <br>  <br><br>`wl_surface`|**DMA-BUF 文件描述符 (`int fd`)**、`wl_buffer` ID、标脏矩形 `(x, y, w, h)`|
|**3. Wayland IPC $\rightarrow$ Compositor**|Unix Domain Socket + `SCM_RIGHTS` 辅助控制消息 (内核 IPC)|`struct wl_resource`<br><br>  <br>  <br><br>`struct weston_surface`<br><br>  <br>  <br><br>`struct gbm_bo`|Socket 协议数据包、**跨进程传递的 DMA-BUF fd**（非像素本身）|
|**4. Compositor $\rightarrow$ libdrm**|C 语言导出库 API (`libdrm` 函数)|`struct gbm_bo`<br><br>  <br>  <br><br>`drmModeAtomicReqPtr`<br><br>  <br>  <br><br>`uint32_t fb_id / plane_id`|**`fb_id`（显示缓存区 ID）**、属性键值对（`CRTC_X/Y/W/H`，`SRC_X/Y/W/H`）|
|**5. libdrm $\rightarrow$ DRM Kernel**|**System Call: `ioctl()`** (`/dev/dri/card0`)|`struct drm_mode_atomic`<br><br>  <br>  <br><br>`struct drm_mode_obj_set_property`|内核态 IOCTL 参数结构体指针、内存偏移量、格式 Modifier|
|**6. DRM Core $\rightarrow$ Vendor 驱动**|内核 VTable / 函数指针数组 (`struct drm_crtc_helper_funcs`)|`struct drm_atomic_state`<br><br>  <br>  <br><br>`struct drm_plane_state`<br><br>  <br>  <br><br>`struct drm_framebuffer`|**显存物理/DMA 总线地址 (`dma_addr_t`)**、硬件 Timing 参数|
|**7. Vendor 驱动 $\rightarrow$ Display HW**|MMIO (Memory-Mapped I/O) 寄存器写入 (`writel()`)|硬件 Register Map (寄存器结构)|**物理内存基地址（物理指针）**、图层控制位|
|**8. Display HW $\rightarrow$ 物理屏幕**|**AXI / AHB 总线 DMA 硬件读取** $\rightarrow$ TMDS / DP 物理总线|物理像素电信号 (RGB / YUV 流)|**真正的全屏物理像素数据流**（由 DMA 抓取）|

## 2. 各层级接口与数据对象深度剖析

### 2.1 Qt 应用层 $\rightarrow$ QPA (Qt Platform Abstraction)

- **接口实现机制**：C++ 类的抽象接口继承与虚函数表（VTable）。
    
      
    
- **数据对象**：
    
      
    - Qt 逻辑层将 `QPushButton` 的重绘指令封装为 `QRegion`。
        
          
        
    - `QWaylandEglWindow` 继承自 `QPlatformWindow`，驱动 EGL 创建 `wl_egl_window` 对象。
        
          
        
- **传输内容**：仅传递绘制指令与局部标脏坐标，GPU 依据这些指令在物理内存（CMA/VRAM）中渲染像素。
    
      
    

### 2.2 Qt QPA $\rightarrow$ Wayland Socket 通信

- **接口实现机制**：`libwayland-client` 提供的 C 语言 Proxy 接口。通过 Unix Domain Socket 传输，利用内核的 **`SCM_RIGHTS` 机制**进行跨进程文件描述符共享。
    
      
    
- **数据对象**：
    
      
    - 客户端暴露 `zwp_linux_dmabuf_v1` 协议对象。
        
          
        
    - 调用 `zwp_linux_dmabuf_v1_create_params()` 将 GPU 渲染好的 GEM 内存导出为 **DMA-BUF 文件描述符 (`int fd`)**。
        
          
        
- **传输内容**：传输 **`fd`**、`format` (如 `DRM_FORMAT_XRGB8888`)、`stride` (跨度) 与 `offset`。**绝不传输像素数组**。
    
      
    

### 2.3 Compositor (窗口合成器) $\rightarrow$ `libdrm`

- **接口实现机制**：C 语言 API 库调用。利用 Mesa 提供的 GBM (Generic Buffer Management) 接口实现内存导入。
    
      
    
- **数据对象**：
    
      
    - 调用 `gbm_bo_import(gbm_dev, GBM_BO_IMPORT_FD_MODIFIER, &fd_data, ...)` 将传进来的 `fd` 还原为 `struct gbm_bo` 对象。
        
          
        
    - 调用 `drmModeAddFB2WithModifiers()` 将 BO 注册到内核，获取一个 **32 位无符号整数 `uint32_t fb_id`**。
        
          
        
    - 构建 `drmModeAtomicReqPtr`（原子提交请求句柄）。
        
          
        
- **传输内容**：`fb_id` 以及图层在屏幕上的位置控制结构（如 `CRTC_ID=54`, `PLANE_ID=40`, `FB_ID=102`, `CRTC_X=0`, `CRTC_Y=0`）。
    
      
    

### 2.4 `libdrm` $\rightarrow$ 内核态 DRM/KMS (Atomic IOCTL)

- **接口实现机制**：Linux 系统调用 **`ioctl(fd, DRM_IOCTL_MODE_ATOMIC, &args)`**。
    
      
    
- **数据对象**：
    
      
    - 用户态 `drmModeAtomicReq` 被打包转化为内核态 `struct drm_mode_atomic` 结构体。
        
          
        
    - 内核解包生成 `struct drm_atomic_state` 全局状态跟踪对象。
        
          
        
- **传输内容**：内存中的结构体指针及属性数组。内核对该结构体进行 `drm_atomic_check_only()` 安全校验。
    
      
    

### 2.5 内核 DRM 子系统 $\rightarrow$ Vendor SoC 显示驱动 (VOP/i915/AMD)

- **接口实现机制**：内核 C 语言结构体中的回调函数指针（VTable 机制），如 `struct drm_plane_helper_funcs` 的 `.atomic_update` 回调。
    
      
    
- **数据对象**：
    
      
    - `struct drm_plane_state` 内部解析 `struct drm_framebuffer`。
        
          
        
    - 通过 DMA-BUF/IOMMU 接口将 `fb` 转换为显存的**物理总线基地址 `dma_addr_t phys_addr`**。
        
          
        
- **传输内容**：纯粹的内存物理基地址（64-bit 整数）与硬件控制掩码。
    
      
    

### 2.6 Vendor 显示驱动 $\rightarrow$ 硬件 display 控制器 (VOP/DC)

- **接口实现机制**：MMIO（内存映射 I/O）。通过内核 `writel(value, reg_base + OFFSET)` 宏直接写芯片物理寄存器。
    
      
    
- **数据对象**：硬件寄存器映射表（Register Map）。
    
      
    
- **传输内容**：将 `phys_addr` 写入显示控制器的 `DMA_ADDR_REG` 寄存器，将分辨率写入 `TIMING_REG` 寄存器。
    
      
    

### 2.7 硬件显示控制器 $\rightarrow$ 物理屏幕

- **接口实现机制**：硬件 AXI/AHB 总线 Master DMA 控制器 + DW-HDMI/DP Transmitter 物理 PHY 芯片。
    
      
    
- **数据对象**：无软件数据结构，转换为总线事务与高频差分电信号。
    
      
    
- **传输内容**：**此时，且仅在此时，DMA 引擎根据写入寄存器的 `phys_addr`，直接从系统 RAM 中拉取真正的物理像素数据字节流**，经过 Serialization 转化为 TMDS/DP 信号发往屏幕。
    
      
    

## 3. 补充“接口与对象传递”的全链路分层架构图

代码段

```
graph TD
    subgraph Layer1 ["1. Qt 应用与 QPA 平台层 (Qt Process)"]
        A[Qt Widget / QML] -->|C++ Call: update| B[QWaylandWindow / QWaylandEglWindow]
        B -->|Mesa EGL API| C[EGLSurface / gbm_bo]
        C -->|Export Handle| D[libwayland-client]
    end

    subgraph Layer2 ["2. IPC 传输层 (Wayland Protocol)"]
        D -->|Unix Socket / SCM_RIGHTS<br>传输 Payload: DMA-BUF int fd| E[Wayland Protocol Requests<br>zwp_linux_dmabuf_v1 / wl_surface.commit]
    end

    subgraph Layer3 ["3. 窗口合成器层 (Wayland Compositor)"]
        E --> F[libwayland-server]
        F -->|C Structure: struct wl_resource| G[Compositor Repaint Engine]
        G -->|C API: gbm_bo_import fd | H[Mesa GBM: struct gbm_bo]
        H -->|C API: drmModeAddFB2WithModifiers | I[libdrm: uint32_t fb_id]
    end

    subgraph Layer4 ["4. 内核 DRM 子系统 (Kernel Space)"]
        I -->|System Call: ioctl DRM_IOCTL_MODE_ATOMIC<br>传输 Payload: struct drm_mode_atomic| J[DRM Core: drm_mode_atomic_ioctl]
        J -->|C VTable: drm_atomic_state| K[DRM Atomic KMS State Machine]
        K -->|C VTable: struct drm_plane_state| L[SoC Driver: .atomic_update]
    end

    subgraph Layer5 ["5. 物理硬件层 (Hardware Layer)"]
        L -->|MMIO Register Write: writel phys_addr<br>传输 Payload: 64-bit 物理内存地址| M[Display Controller IP VOP/DC]
        M -->|AXI Bus DMA Fetch<br>传输 Payload: 真正的物理像素字节流| N[HDMI/DP Transmitter PHY]
        N -->|TMDS/DP Differential Signal| O[Physical Monitor 屏幕]
    end
```

## 4. 标注接口机制与数据对象的 UML 序列图

以下序列图详细标记了**每个调用步骤所使用的 C/C++ 接口函数、系统调用以及传递的具体数据结构/对象**：

代码段

```
sequenceDiagram
    autonumber
    actor User as 用户
    participant QtApp as Qt App (QWidget)
    participant QPA as Qt QPA (qwayland-egl)
    participant Socket as Unix Domain Socket
    participant Compositor as Wayland Compositor
    participant LibDRM as libdrm API
    participant DRM_Kernel as DRM Core (Kernel)
    participant HW_VOP as Display HW (VOP/DC)

    %% 1. Qt 内部
    User->>QtApp: 点击按钮 (QPushButton)
    QtApp->>QtApp: mouseReleaseEvent() -> QWidget::update(QRegion)
    QtApp->>QPA: QWaylandWindow::requestUpdate()
    Note over QPA: GPU 渲染像素至显存<br>调用 eglSwapBuffers()
    QPA->>QPA: gbm_bo_get_fd() -> 导出 DMA-BUF int fd

    %% 2. IPC 传输
    Note over QPA,Socket: 接口: libwayland-client API<br>对象: zwp_linux_dmabuf_v1<br>Payload: int fd + 坐标元数据 (无像素)
    QPA->>Socket: zwp_linux_dmabuf_v1.create_params(fd, format, modifier)
    QPA->>Socket: wl_surface.attach(wl_buffer) & wl_surface.commit()

    %% 3. Compositor 处理
    Socket->>Compositor: SCM_RIGHTS 接收 Socket 字节流与 int fd
    Note over Compositor: 接口: Mesa GBM C API<br>对象: struct gbm_bo, uint32_t fb_id
    Compositor->>Compositor: gbm_bo_import(GBM_BO_IMPORT_FD_MODIFIER, &fd_data)
    Compositor->>LibDRM: drmModeAddFB2WithModifiers(drm_fd, ..., &fb_id)
    LibDRM-->>Compositor: 返回 DRM Framebuffer ID (uint32_t fb_id)

    %% 4. 提交给内核 DRM
    Compositor->>LibDRM: drmModeAtomicAlloc() -> drmModeAtomicAddProperty()
    Note over Compositor,DRM_Kernel: 接口: system call ioctl(DRM_IOCTL_MODE_ATOMIC)<br>对象: struct drm_mode_atomic<br>Payload: fb_id + 图层坐标属性 (无像素)
    Compositor->>LibDRM: drmModeAtomicCommit(drm_fd, atomic_req, flags, user_data)
    LibDRM->>DRM_Kernel: ioctl(drm_fd, DRM_IOCTL_MODE_ATOMIC, struct drm_mode_atomic *args)

    %% 5. 内核态处理与 MMIO
    rect rgb(235, 245, 255)
        Note over DRM_Kernel: 内核态驱动处理 (VTable 回调)<br>对象: struct drm_atomic_state
        DRM_Kernel->>DRM_Kernel: drm_atomic_check_only()
        DRM_Kernel->>DRM_Kernel: drm_buf_to_phys_addr() -> 解析得到 dma_addr_t
        DRM_Kernel->>HW_VOP: Driver .atomic_update() 回调
    end

    %% 6. 硬件写寄存器与 DMA 抓取
    Note over DRM_Kernel,HW_VOP: 接口: MMIO 寄存器写入 writel()<br>Payload: 64-bit 物理内存地址 phys_addr
    DRM_Kernel->>HW_VOP: writel(phys_addr, VOP_WIN0_YRGB_MST0)
    
    rect rgb(255, 240, 245)
        Note over HW_VOP: 硬件层：真正的像素传输开始！
        HW_VOP->>HW_VOP: DMA 引擎读取 RAM 中的物理像素流
        HW_VOP-->>User: 通过 HDMI/DP 物理线缆输出高低电信号，屏显更新
    end
```