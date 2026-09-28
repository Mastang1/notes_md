# Linux 现代图形栈全栈架构与数据流技术笔记（从 GUI 框架到 DRM/KMS）

---

## 1. 全栈架构与旁系子模块总览

Linux 现代图形栈遵循 **“Smart User-Space, Dumb Kernel”** 的设计哲学。
_**上层应用程序负责像素内容生产与逻辑合成，窗口合成器负责多窗口拓扑与画面混合，内核 DRM 子系统仅负责显存生命周期与硬件显示管线的控制。

### 1.1 Linux 现代图形栈总体架构图

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                     USER SPACE (用户态)                                                │
│                                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 应用与 GUI 框架层 (Client Application Process)                                                   │  │
│  │                                                                                                  │  │
│  │  ┌────────────────────────┐         ┌────────────────────────┐         ┌──────────────────────┐  │  │
│  │  │  Qt6 (QWidget / QML)   │         │  GTK4 (GtkWidget/GSK)  │         │ Flutter / Chromium   │  │  │
│  │  │  [In-Process Widgets]  │         │  [In-Process Widgets]  │         │ [In-Process Canvas]  │  │  │
│  │  └───────────┬────────────┘         └───────────┬────────────┘         └───────────┬──────────┘  │  │
│  │              │ (Offscreen Paint)                │ (Offscreen Paint)                │             │  │
│  │              ▼                                  ▼                                  ▼             │  │
│  │    [ Top-Level QWindow ]              [ Top-Level GdkSurface ]           [ Top-Level Canvas ]    │  │
│  └──────────────┬──────────────────────────────────┬──────────────────────────────────┬─────────────┘  │
│                 │ (QPA Platform Plugin)            │ (GSK Backend)                    │                │
│                 ▼                                  ▼                                  ▼                │
│  ┌──────────────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 窗口系统协议客户端与 CPU/GPU 渲染引擎                                                            │  │
│  │                                                                                                  │  │
│  │  【旁系 CPU 软绘图引擎】                      【主线 GPU 硬件加速渲染引擎】                      │  │
│  │  ┌────────────────────────┐                 ┌────────────────────────────────────────┐           │  │
│  │  │ Pixman / Cairo / Skia  │                 │ Mesa 3D Driver (Iris / RadeonSI / Pan) │           │  │
│  │  │ (Software Rasterizer)  │                 │ (OpenGL ES / Vulkan Backend)           │           │  │
│  │  └───────────┬────────────┘                 └───────────────────┬────────────────────┘           │  │
│  │              │ (CPU Write)                                      │ (GPU Command Stream)           │  │
│  │              ▼                                                  ▼                                │  │
│  │  ┌────────────────────────┐                 ┌────────────────────────────────────────┐           │  │
│  │  │ SHM (Shared Memory)    │                 │ EGL / GBM (Generic Buffer Management)  │           │  │
│  │  │ wl_shm_pool            │                 │ libgbm.so / dma_buf Allocation         │           │  │
│  │  └───────────┬────────────┘                 └───────────────────┬────────────────────┘           │  │
│  └──────────────┼──────────────────────────────────────────────────┼────────────────────────────────┘  │
│                 │ (Shared Memory Pointer)                          │ (dma_buf File Descriptor)         │
│                 │                                                  │                                   │
│                 │    IPC Protocol Channel (Unix Domain Socket)     │                                   │
│                 │    Protocol: libwayland-client ──► libwayland-server                              │
│                 │    Interfaces: wl_surface, xdg_toplevel, zwp_linux_dmabuf_v1                        │
│                 │                                                  │                                   │
│                 ▼                                                  ▼                                   │
│  ┌──────────────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 窗口合成服务器层 (Wayland Compositor / Display Server Process)                                    │  │
│  │ (Mutter / KWin / Weston / Sway)                                                                  │  │
│  │                                                                                                  │  │
│  │  ┌────────────────────────────────────────┐      ┌────────────────────────────────────────────┐  │  │
│  │  │ 场景图管理 (Scene Graph / Z-Order)     │      │ 增量更新计算 (Damage Region Math)          │  │  │
│  │  │ - Surface Tree (weston_surface)        │      │ - Pixman Region (pixman_region32_t)        │  │  │
│  │  └───────────────────┬────────────────────┘      └─────────────────────┬──────────────────────┘  │  │
│  │                      └─────────────────────────┬───────────────────────┘                         │  │
│  │                                                ▼                                                 │  │
│  │  【合成路径决策引擎】                                                                            │  │
│  │  ├─ 路径 A (多窗口/混合特效): 调 Mesa GLES 引擎将多个 Client dma_buf 合成至 Compositor Buffer    │  │
│  │  └─ 路径 B (全屏 Direct Scanout): 跳过 GPU 合成，直接将 Client dma_buf 交付内核 Hardware Plane   │  │
│  └────────────────────────────────────────────────┬─────────────────────────────────────────────────┘  │
│                                                   │                                                    │
│                                                   │ User-space DRM Control API                         │
│                                                   │ Wrapper: libdrm.so                                 │
│                                                   ▼                                                    │
├────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│                                     KERNEL SPACE (内核态)                                              │
│                                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ Direct Rendering Manager (DRM) 子系统                                                            │  │
│  │                                                                                                  │  │
│  │  【GEM (Graphics Execution Manager)】            【KMS (Kernel Mode Setting)】                   │  │
│  │  ├─ 显存/内存分配与生命周期管理                  ├─ drm_framebuffer (像素点阵描述)               │  │
│  │  ├─ dma_buf 导出/导入 (PRIME 跨进程共享)          ├─ drm_plane (Primary / Overlay / Cursor 图层)  │  │
│  │  └─ Sync File / dma_fence (GPU/CPU 渲染同步)      ├─ drm_crtc (扫描引擎 & VSYNC 时钟控制)          │  │
│  │                                                  ├─ drm_encoder (信号协议编码: TMDS/DP/DSI)       │  │
│  │                                                  └─ drm_connector (物理接口与 EDID 检测)         │  │
│  └────────────────────────────────────────────────┬─────────────────────────────────────────────────┘  │
│                                                   │                                                    │
├───────────────────────────────────────────────────┼────────────────────────────────────────────────────┤
│                                     HARDWARE LAYER (硬件层)                                            │
│                                                   ▼                                                    │
│  ┌────────────────────────┐         ┌────────────────────────┐         ┌────────────────────────────┐  │
│  │ System RAM / VRAM      │◄───────┤ GPU Execution Engine   │         │ Display Engine / Controller│  │
│  │ (渲染与合成点阵存储)   │         │ (3D/Compute Pipeline)  │         │ (CRTC DMA & Hardware Blender)│  │
│  └────────────────────────┘         └────────────────────────┘         └──────────────┬─────────────┘  │
│                                                                                       │                │
│                                                                                       ▼                │
│                                                                        ┌────────────────────────────┐  │
│                                                                        │ Display Panel (显示器面板) │  │
│                                                                        └────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

```

---

### 1.2 对应层级的 Linux 系统标准动态库列表

| 架构层级            | 核心模块 / 角色                   | 常用 Linux 标准动态库 / 模块名称                                                   | 库的主要职责与功能                                                               |
| --------------- | --------------------------- | ----------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| **应用与 GUI 框架层** | 顶层 UI 控件库                   | `libQt6Widgets.so`, `libQt6Gui.so`, `libgtk-4.so`                       | 维护 Widget/DOM 树，响应输入事件，计算本地 Damage 损伤区域。                                |
| **平台抽象后端**      | Platform Plugin             | `libqwayland-egl.so` (Qt QPA), `libgdk-3.so`                            | 连接 GUI 框架与底层 Native 窗口系统，将 `QWindow` 抽象映射为 `wl_surface`。                |
| **CPU 绘图旁系子模块** | 软件渲染库                       | `libpixman-1.so`, `libcairo.so`, `libSkia.so`                           | 当无 GPU 硬件加速时，利用 CPU SIMD 指令执行点阵像素绘制与区域几何剪裁计算。                           |
| **IPC 协议库**     | Wayland 核心 C 库              | `libwayland-client.so`, `libwayland-server.so`                          | 提供 Unix Domain Socket 之上的 Wayland binary 协议序列化与反序列化支持。                  |
| **窗口合成服务器**     | Compositor / Display Server | `mutter` (GNOME), `kwin_wayland` (KDE), `weston`                        | 独立的 User-Space 守护进程，维护场景图，处理 Z-Order，执行帧合成与 Direct Scanout。             |
| **显存与硬件抽象接口**   | 显存与 EGL 胶水层                 | `libgbm.so`, `libEGL_mesa.so`, `libGLX_mesa.so`                         | **GBM**：通用缓冲区管理，分配带 GPU/Display 控制器标志的内存；**EGL**：渲染 API 与原生窗口的桥梁。       |
| **用户态 GPU 驱动**  | Mesa 3D Driver              | `iris_dri.so` (Intel), `radeonsi_dri.so` (AMD), `panfrost_dri.so` (ARM) | 将 OpenGL ES / Vulkan 着色器与 Draw Calls 编译为特定 GPU 架构的微码与硬件 Command Buffer。 |
| **DRM 用户态封装**   | Kernel API Wrapper          | `libdrm.so` (`libdrm_intel.so`, `libdrm_amdgpu.so`)                     | 对 DRM 内核字符设备（`/dev/dri/card0`）的 `ioctl` 系统调用进行 C 语言接口封装。                |
| **内核 DRM 子系统**  | Kernel Modules              | `drm.ko`, `drm_kms_helper.ko`, `i915.ko`, `xe.ko`, `amdgpu.ko`          | 驱动显卡硬件，管理 GEM 显存、处理 KMS 物理屏管线配置，响应 VSYNC 物理中断。                          |

---

## 2. 分层语义模型与核心对象解构

### Layer 1: 应用与 UI 框架层 (Client Application Layer)

* **语义职责**：处理应用业务逻辑、DOM/控件树布局以及事件响应。
* **Alien Widgets 机制**：
* 在 Qt 或 GTK 等现代框架中，`QPushButton`、`QLineEdit`、`QDialog` 等非顶层控件被称为 **Alien Widgets**。
* **重要事实**：子控件**没有**独立的 Native Window。GUI 框架会在应用进程内把所有子控件离屏绘制（Off-screen Rendering）并合成到同一个顶层 **`QWindow`** 内存画布中。


* **数据结构**：`QWidget` 控件树、`QPainter` 命令队列、`QRegion`（应用内部损伤区域）。

### Layer 2: 平台抽象与 Native 协议通道层 (IPC Protocol & Platform Abstraction)

* **语义职责**：隔离不同操作系统的窗口系统差异（Wayland, X11, DirectFB, macOS Cocoa），并将应用内部的 `QWindow` 映射为 Native 窗口句柄。
* **核心协议对象（Wayland 规范）**：
* **`wl_compositor`**：合成器全局单例，用于创建 `wl_surface`。
* **`wl_surface`**：客户端在 Server 端建立的矩形画布节点。
* **`xdg_toplevel`**：由 `xdg_shell` 扩展提供，赋予 `wl_surface` 基础桌面窗口属性（最大化、最小化、标题栏、调整尺寸）。
* **`zwp_linux_dmabuf_v1`**：零拷贝共享内存扩展协议，允许 Client 直接传递显存 `dma_buf fd` 给 Server。



### Layer 3: 窗口合成服务器层 (Wayland Compositor Layer)

* **语义职责**：作为系统的 Display Server 守护进程，管理桌面上所有应用窗口的拓扑关系，并执行合成。
* **核心数据结构与算法**：
* **Scene Graph (场景图)**：如 Weston 中的 `struct weston_surface` 与 `struct weston_view` 构成的双向链表/树，维护全局窗口的坐标映射（$x, y, w, h$）与 Z-Order（深度层级）。
* **Damage Region 合并（Pixman）**：利用 `libpixman-1` 提供的 `pixman_region32_t` 执行二元集合运算（Union / Intersect / Subtract），精准计算出桌面发生改变的脏区域。
* **Alpha Blending 混合计算**：若窗口包含半透明属性，Compositor 调用 GPU 片段着色器，执行 Porter-Duff 算法：

$$C_{\text{result}} = C_{\text{src}} \cdot A_{\text{src}} + C_{\text{dst}} \cdot (1 - A_{\text{src}})$$





### Layer 4: 用户态驱动与显存抽象层 (Mesa 3D & EGL / GBM)

* **语义职责**：提供硬件无关的图形 API 抽象，并将像素内存抽象为可在不同进程与硬件模块间共享的 Handle。
* **核心概念与数据结构**：
* **EGL**：Khronos 渲染 API（OpenGL ES）与 Native 窗口系统间的粘合层。`eglCreateWindowSurface()` 将 Native 窗口句柄绑定为 EGL 渲染目标。
* **GBM (Generic Buffer Management)**：通过 `gbm_surface_create()` 分配跨硬件兼容的显存 Buffer Object (`gbm_bo`)。
* **DMA-BUF (Direct Memory Access Buffer)**：Linux 内核提供的跨进程/跨设备显存共享机制。Mesa 调用 `drmPrimeHandleToFD()` 将 GEM 句柄转换为全局唯一的 File Descriptor (`dma_buf fd`)。



### Layer 5: 内核态 DRM 子系统 (Direct Rendering Manager)

* **语义职责**：独占驱动显示硬件，控制显存分配与物理显示管线。

* **GEM (Graphics Execution Manager)**：管理显存（VRAM 或 System RAM DMA 页），处理内存映射、Cache 一致性及 GPU/CPU 同步（`dma_fence`）。
* **KMS (Kernel Mode Setting) 五大核心内核对象**：
1. **`drm_framebuffer`**：描述显存点阵格式的元数据对象（存储 Pixel Format 如 `DRM_FORMAT_XRGB8888`、Pitch/Stride、Offset 及绑定的 `dma_buf`）。
2. **`drm_plane`**：硬件合成图层通道。主要包括：
* `DRM_PLANE_TYPE_PRIMARY`：承载主桌面合成帧。
* `DRM_PLANE_TYPE_OVERLAY`：硬件 Video/Sprite 叠加图层（专用于 Direct Scanout）。
* `DRM_PLANE_TYPE_CURSOR`：独立硬件鼠标光标图层。


3. **`drm_crtc`**：显示控制器逻辑抽象。负责读取 `drm_plane` 像素流，按 Timing 时钟生成行/场同步信号与 VSYNC 中断。
4. **`drm_encoder`**：将 CRTC 输出的并行 RGB 像素流编码转换为 TMDS, DisplayPort 或 MIPI DSI 物理协议信号。
5. **`drm_connector`**：物理输出接口抽象（如 HDMI-A-1, DP-1）。负责读取显示器 EDID 数据包，并响应 HPD (Hot Plug Detect) 热插拔中断。



---

## 3. 场景序列图一：GUI 框架内部离屏合成序列

此序列描述应用进程空间内部（In-Process），GUI 框架（以 Qt 为例）如何将离散的子控件（Alien Widgets）绘制并离屏合成到顶层 `QWindow` 绑定的 `dma_buf` 内存缓冲区中。

```
┌─────────────┐            ┌──────────────┐          ┌────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ User Event /│            │ Top-Level    │          │ Alien Widgets  │       │ Qt RHI / Mesa   │       │ EGL / GBM       │
│ Repaint Req │            │ QWindow      │          │ Tree (Child)   │       │ GLES Engine     │       │ Memory Allocator│
└──────┬──────┘            └──────┬───────┘          └───────┬────────┘       └────────┬────────┘       └────────┬────────┘
       │                          │                          │                         │                         │
       │ 1. Trigger update()      │                          │                         │                         │
       ├─────────────────────────►│                          │                         │                         │
       │                          │ 2. Calculate Damage      │                         │                         │
       │                          │    Region (QRegion)      │                         │                         │
       │                          ├─────────────────────────►│                         │                         │
       │                          │                          │ 3. Execute paintEvent() │                         │
       │                          │                          │    (Recurse Widget Tree)│                         │
       │                          │                          │                         │                         │
       │                          │ 4. Acquire Back Buffer   │                         │                         │
       │                          ├──────────────────────────┼─────────────────────────┼────────────────────────►│
       │                          │                          │                         │                         │ 5. gbm_bo_create() /
       │                          │                          │                         │                         │    Allocate dma_buf
       │                          │                          │                         │                         │◄────────────────────────┘
       │                          │ 6. Return EGLSurface / dma_buf fd                  │                         │
       │                          │◄───────────────────────────────────────────────────┴─────────────────────────┤
       │                          │                                                                              │
       │                          │ 7. Bind Render Target (glFramebufferTexture2D)                               │
       │                          ├────────────────────────────────────────────────────►│                         │
       │                          │                                                    │                         │
       │                          │ 8. Issue Draw Calls (Render Buttons, Text, Views)  │                         │
       │                          ├──────────────────────────┬────────────────────────►│                         │
       │                          │                          │                         │ 9. Execute Rasterization│
       │                          │                          │                         │    Write Pixels to      │
       │                          │                          │                         │    dma_buf Memory       │
       │                          │                          │                         │                         │
       │                          │ 10. Frame Render Complete│                         │                         │
       │                          │◄─────────────────────────┴─────────────────────────┤                         │
       │                          │                                                                              │
       │                          │ 11. Export dma_buf fd via drmPrimeHandleToFD()                              │
       │                          ├─────────────────────────────────────────────────────────────────────────────►│
       │                          │                                                                              │
       │                          │ 12. Off-screen Buffer Ready (In-Process Composition Complete)                │
       │                          │ (Next: Ready for Wayland IPC Commit)                                         │

```

---

## 4. 场景序列图二：从 GUI 框架到 DRM KMS 端到端数据流

此序列展示当应用完成内部帧渲染后，像素数据句柄通过 Wayland IPC 协议跨进程传输至 Compositor，进而触发 GPU 合成/Direct Scanout，最终交由内核 DRM KMS 执行硬件扫描显示的完整端到端流程。

```
┌───────────────┐           ┌──────────────────┐       ┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ Qt App Client │           │ Wayland Protocol │       │ Wayland         │       │ Mesa / EGL      │       │ DRM / KMS       │
│ Process       │           │ IPC Socket       │       │ Compositor      │       │ Driver (User)   │       │ Kernel Module   │
└───────┬───────┘           └────────┬─────────┘       └────────┬────────┘       └────────┬────────┘       └────────┬────────┘
        │                            │                          │                         │                         │
        │ 1. zwp_linux_dmabuf_v1.create_params()                │                         │                         │
        ├───────────────────────────►│                          │                         │                         │
        │ 2. Pass dma_buf fd         │ 3. Dispatch Event        │                         │                         │
        ├───────────────────────────►├─────────────────────────►│                         │                         │
        │                            │                          │ 4. Import dma_buf fd    │                         │
        │                            │                          │    eglCreateImageKHR()  │                         │
        │                            │                          ├────────────────────────►│                         │
        │ 5. wl_surface.damage_buffer(x, y, w, h)               │                         │                         │
        ├───────────────────────────►│                          │                         │                         │
        │ 6. wl_surface.commit()     │                          │                         │                         │
        ├───────────────────────────►│ 7. Receive Commit        │                         │                         │
        │                            ├─────────────────────────►│                         │                         │
        │                            │                          │                         │                         │
        │                            │                          │ 8. Scene Graph &        │                         │
        │                            │                          │    Pixman Damage Math   │                         │
        │                            │                          │                         │                         │
        │                            │                          │ 9. Evaluate Composition Path                      │
        │                            │                          ├───────────────────────────────────────────────────┤
        │                            │                          │                                                   │
        │                            │                          ├───► [Path A: Multi-Window / Alpha Blending]       │
        │                            │                          │     - Render all App Buffers into                 │
        │                            │                          │       Compositor Back Buffer using GPU GLES       │
        │                            │                          │                                                   │
        │                            │                          └───► [Path B: Direct Scanout (Fullscreen App)]     │
        │                            │                                - Bypass GPU Blend, pass App dma_buf          │
        │                            │                                  directly to DRM Hardware Overlay Plane      │
        │                            │                                                                              │
        │                            │                          │ 10. drmModeAddFB2WithModifiers()                  │
        │                            │                          ├──────────────────────────────────────────────────►│
        │                            │                          │     Wrap dma_buf as drm_framebuffer               │
        │                            │                          │                                                   │
        │                            │                          │ 11. DRM_IOCTL_MODE_ATOMIC                         │
        │                            │                          │     (Atomic KMS State Commit)                     │
        │                            │                          ├──────────────────────────────────────────────────►│
        │                            │                          │     - Set Plane -> FB_ID                          │
        │                            │                          │     - Set Plane -> CRTC_ID                        │
        │                            │                          │     - Set Mode Timing                             │
        │                            │                          │                                                   │
        │                            │                          │                                                   │ 12. Hardware Scanout
        │                            │                          │                                                     (CRTC DMA Engine)
        │                            │                          │                                                     Pixel Clock Read Memory
        │                            │                          │                                                     Plane -> CRTC -> Enc -> Conn
        │                            │                          │                                                   │
        │                            │                          │ 13. Hardware VSYNC Interrupt                      │
        │                            │                          │◄──────────────────────────────────────────────────┤
        │                            │                          │                                                   │
        │                            │ 14. wl_surface.frame()   │                                                   │
        │                            │     Send Callback        │                                                   │
        │◄───────────────────────────┴──────────────────────────┤                                                   │
        │                                                       │                                                   │
        │ 15. Trigger Next Frame Draw Cycle                     │                                                   │

```

---

## 5. 全栈机制核心总结

1. **控件非 Native 窗口（Alien Widgets）**：UI 框架（Qt/GTK）在进程内部完成内部控件树的离屏绘制，向 Native 窗口系统注册并交互的最小单位是顶层 **`QWindow` / `wl_surface**`。
2. **零拷贝（Zero-Copy）数据流**：Client 与 Compositor 之间传输的不是原始像素点阵，而是基于 **`dma_buf`** 机制的物理显存文件描述符（FD）。
3. **两级合成解耦**：
* **进程内合成（In-Process）**：GUI 框架利用 CPU（Pixman/Cairo）或 GPU（Mesa/GLES）将应用内部所有 Widget 画入 `QWindow` 的 `dma_buf`。
* **显示合成（Display Composition）**：窗口合成器（Compositor）维护全局场景图，通过 **GPU 合成** 或 **DRM Plane 直通（Direct Scanout）** 将不同 App 的 `dma_buf` 投递至显示硬件面板。