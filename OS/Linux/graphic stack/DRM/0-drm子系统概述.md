
# DRM 子系统核心功能与架构组成 Summary

## 1. 核心四大功能模块

| **功能模块**                                                | **核心职责**                                                             | **依赖的关键技术/组件**                                                                    |
| ------------------------------------------------------- | -------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| **1. 显存与渲染管理**<br><br>  <br>  <br><br>_(Data Path)_     | 负责 GPU/ display 控制器所需的显存分配、虚拟内存映射、CPU/GPU 缓存一致性同步以及算力命令队列调度。         | **GEM** (Graphics Execution Manager)<br><br>  <br>  <br><br>**TTM** (针对独立显卡 VRAM) |
| **2. 显示管道管理**<br><br>  <br>  <br><br>_(Control Path)_   | 负责显示分辨率、刷新率设置、时序生成（HSYNC/VSYNC）、多图层硬件 Alpha 合成与缩放。                   | **KMS** (Kernel Mode Setting)<br><br>  <br>  <br><br>**Atomic KMS** 状态机           |
| **3. 接口与协议转换**<br><br>  <br>  <br><br>_(Output Path)_   | 负责将像素流转码为标准协议信号（HDMI TMDS、DP SST/MST、VGA 模拟信号等），处理 HPD 热插拔与 EDID 读取。 | **Connector / Bridge / Panel** 辅助框架<br><br>  <br>  <br><br>DDC (I2C) / HDCP       |
| **4. 跨设备零拷贝**<br><br>  <br>  <br><br>_(Buffer Sharing)_ | 实现 GEM Buffer 在 GPU 渲染器、VPU 硬件解码器、Camera 与 Display 驱动之间的跨设备零拷贝共享。    | **DMA-BUF / PRIME** 机制                                                            |

## 2. DRM 架构分层视图

从用户态到底层硬件，DRM 的控制流与数据流逻辑如下：

  

```
+-----------------------------------------------------------------------+
| 用户空间 (User Space)                                                  |
| Display Server (Wayland/Weston) / Mesa 3D / GStreamer / App           |
+-----------------------------------------------------------------------+
        |  (open / ioctl / mmap / poll)
        v
+-----------------------------------------------------------------------+
| Linux VFS 层 (/dev/dri/)                                               |
|  ├── /dev/dri/card0       (Primary Node: 包含 KMS 显示控制 + GEM 显存) |
|  └── /dev/dri/renderD128  (Render Node: 纯 GPGPU / 3D 渲染 / 离屏计算)  |
+-----------------------------------------------------------------------+
        |  (drm_fops -> drm_ioctl)
        v
+-----------------------------------------------------------------------+
| DRM 核心子系统 (DRM Core)                                              |
|  ├── GEM Core         (显存生命周期、mmap、DMA-BUF 导出)                 |
|  ├── KMS Core         (Atomic Check/Commit 状态管理、IDR 对象树)         |
|  └── DRM Helpers      (drm_bridge_chain, drm_panel, drm_edid_parser)  |
+-----------------------------------------------------------------------+
        |  (回调驱动注册的 drm_***_funcs 函数指针)
        v
+-----------------------------------------------------------------------+
| SoC / Vendor 厂商驱动层 (drivers/gpu/drm/<vendor>/)                    |
|  ├── Master Driver    (Component Aggregator 组装主驱动)               |
|  ├── Display Driver   (CRTC / Plane: 产生时序并进行图层 DMA 抓取)        |
|  └── Output Driver    (HDMI / VGA / DP Controller IP & PHY 寄存器配置)   |
+-----------------------------------------------------------------------+
        |  (MMIO / I2C / 中断)
        v
+-----------------------------------------------------------------------+
| 物理硬件层 (Hardware Layer)                                           |
| VOP / Display Controller | DW-HDMI IP | PHY | HDMI/VGA 显示器         |
+-----------------------------------------------------------------------+
```

## 3. KMS 五大核心数据结构 (KMS Mode Objects)

在 DRM 的显示控制（KMS）中，所有被用户态通过 `obj_id` 操纵的元素，均继承自抽象基类 `struct drm_mode_object`：

  

```
[ Framebuffer ] ──> [ Plane ] ──> [ CRTC ] ──> [ Encoder ] ──> [ Connector ] ──> (Monitor)
   (显存元数据)       (图层混合)   (时序产生)     (信号编码)       (物理接口)
```

1. **`struct drm_framebuffer` (FB)**：
    
      
    - **功能**：抽象一块装有像素数据的内存，绑定具体的 GEM 对象，记录 Width、Height、Pitch、Pixel Format（如 NV12、ARGB8888）。
        
          
        
2. **`struct drm_plane` (图层)**：
    
      
    - **功能**：抽象硬件图层/混合器（Overlay/Primary/Cursor）。负责从 Framebuffer 提取数据，完成裁切、缩放、Alpha 混合后送到 CRTC。
        
          
        
3. **`struct drm_crtc` (显示控制器)**：
    
      
    - **功能**：抽象显示控制器 Timing 发生器。负责控制分辨率与帧率，产生 HSYNC/VSYNC，将 Plane 叠加后的画面扫描输出到 Encoder。
        
          
        
4. **`struct drm_encoder` (编码器)**：
    
      
    - **功能**：抽象信号转换逻辑。将 CRTC 的并行 RGB/YUV 像素流编码转换为具体接口所需的串行协议流（如 HDMI TMDS）。
        
          
        
5. **`struct drm_connector` (物理连接器)**：
    
      
    - **功能**：抽象物理插口（如 HDMI-A 口、VGA D-Sub 口）。负责维护连接状态（Connected/Disconnected）、响应 HPD 热插拔中断，并通过 DDC 通道读取解析显示器的 EDID。
        
          
        

> **注**：在内核驱动内部，对于复杂的编码器 IP（如 DesignWare HDMI），通常会使用 **`struct drm_bridge`** 框架将 Encoder 的控制逻辑与具体 PHY / 芯片级联解耦（Bridge 对用户态不可见）。
> 
>   

## 4. 两类设备节点与权限隔离

DRM 在 VFS 中通过字符设备的形式向外部暴露接口，按权限与功能划分为两类节点：

|**节点路径**|**内核类型定义**|**具备的权限**|**典型使用者**|
|---|---|---|---|
|**`/dev/dri/cardX`**|`DRM_MINOR_PRIMARY`|**KMS 显示控制** + **GEM 显存分配** + **渲染**|Wayland / Weston / X11 等 Display Server（占用 KMS Master 独占锁）。|
|**`/dev/dri/renderD128+`**|`DRM_MINOR_RENDER`|**GEM 显存分配** + **GPU/VPU 硬件计算**（无 KMS 控制权）|普通 App、GStreamer 硬件解码、OpenGL/Vulkan 客户端、AI 推理程序。|
