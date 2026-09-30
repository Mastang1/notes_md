# Q:
我此前理解drm的子设备是指vga、多个hdmi接口或者其他接口这种同功能的设备，但是看你的回答，是把不同的组件划分为了drm的子设备，也就是实际工作时候，drm根据下发的命令和配置，会调用不同的子设备对吗？

# A:

**对，您的理解完全准确！**

  

DRM 框架中的“子设备”（在 KMS 中表现为各种 `drm_mode_object`），不仅包含您之前理解的**横向多接口拓展**（如 HDMI-1, HDMI-2, DP-1），更核心的是将**一条单向显示数据流水线纵向拆解**出来的各个功能 IP 组件（Plane, CRTC, Encoder, Bridge, Connector）。

  

当用户态下发渲染或显示命令时，DRM 会精准按需调用流水线上各个子设备对应的驱动接口。

  

### 1. 双重维度的“子设备”拆分

|**拆分维度**|**包含的子设备/组件**|**解决什么问题**|
|---|---|---|
|**纵向拆分**<br><br>  <br>  <br><br>_(硬件流水线组件)_|`Plane` (图层) $\rightarrow$ `CRTC` ( Timing 时序) $\rightarrow$ `Encoder/Bridge` (信号转换) $\rightarrow$ `Connector` (接口)|解决同一条流水线内部各个 IP 硬件的**功能解耦**与**动态拼接**。|
|**横向拆分**<br><br>  <br>  <br><br>_(同类多路拓展)_|`CRTC_0` / `CRTC_1`<br><br>  <br>  <br><br>`HDMI-A-1` / `HDMI-A-2` / `DP-1`|解决 SoC 支持**多屏异显**、**多接口同时输出**时的独立资源调度。|

### 2. 实际工作时：DRM 如何根据命令调用不同的子设备？

当用户态（如 Display Server）通过 `DRM_IOCTL_MODE_ATOMIC` 下发一帧画面配置时，DRM 核心会将这个配置解构成针对各个子设备的操作，并按固定顺序调用它们的驱动回调：

  

```
                             用户态下发 Atomic Commit 指令
                                         │
                                         ▼
                             DRM 核心 (Atomic 状态机)
                                         │
     ┌───────────────────┬───────────────┴───────────────┬───────────────────┐
     ▼                   ▼                               ▼                   ▼
 Plane 子设备        CRTC 子设备                     Bridge 子设备        Connector 子设备
(图层 DMA/混合)    (Timing/显示时钟)               (HDMI 协议/PHY)      (接口/EDID/HPD)
     │                   │                               │                   │
     ▼                   ▼                               ▼                   ▼
调用驱动回调:       调用驱动回调:                   调用驱动回调:        调用驱动回调:
.atomic_update()    .atomic_enable()                .atomic_enable()     .detect() / .get_modes()
     │                   │                               │                   │
     ▼                   ▼                               ▼                   ▼
配置 VOP 图层寄存器   配置 VOP 时序与时钟             配置 DW-HDMI 寄存器   通过 DDC 读 I2C / 监测 HPD
```

#### 具体调用逻辑示例：

1. **只改画面内容/位置**：如果只是 UI 界面刷新，显示分辨率没变，DRM **只会调用 `Plane` 子设备**的 `.atomic_update()` 回调去修改 DMA 显存地址，根本不会打扰 `CRTC` 和 `Bridge`。
    
      
    
2. **切换分辨率/刷新率**：如果用户切了分辨率，DRM 会先调用 `CRTC` 子设备重新配置像素时钟（Pixel Clock），再调用 `Bridge` 子设备重新做 HDMI TMDS 编码匹配。
    
      
    
3. **拔插线缆**：当 HDMI 线被拔出，`Connector` 子设备触发 HPD 中断，DRM 捕获后通知用户态；用户态关闭输出，DRM 会沿路调用 `Bridge` $\rightarrow$ `CRTC` $\rightarrow$ `Plane` 的 `.atomic_disable()` 回调，依序关闭各个子设备的硬件电源与时钟。
    
      
    

这种按需调用的设计，保证了显示内核在处理极高频的帧刷新（如 60Hz/120Hz）时，只更新变动的硬件 IP，运行效率极高。