恭喜顺利点亮 HDMI 屏幕！以下为您整理的修改点总结及 ISP Demo 开发笔记，方便后续调试与维护：

  

# Sophon ISP Camera Demo 开发笔记

### 1. 修改点总结

为了将摄像头视频流从单纯的网络 RTSP 推流切换/叠加到本地 HDMI 屏幕显示，仅需要修改 **主配置文件** 中的 HDMI 使能开关：

  

- **修改文件**：`/opt/sophon/sophon-soc-libisp_1.0.0/bin/cfg.json`
    
      
    
- **修改字段**：`"enable-hmdi"` (或 `"enable-hdmi"`) 调整为 `1`
    
      
    
- **快捷修改命令**：
    
      
    
    Bash
    
    ```
    sed -i 's/"enable-hmdi":[[:space:]]*0/"enable-hmdi": 1/g' /opt/sophon/sophon-soc-libisp_1.0.0/bin/cfg.json
    ```
    

### 2. 核心配置文件说明

|**配置文件**|**作用描述**|**关键参数/字段**|
|---|---|---|
|**`cfg.json`**|**系统及视频源主配置文件**<br><br>  <br>  <br><br>控制视频输入通道、图像分辨率、全局硬件输出管道使能等。|- `enable-hmdi`: HDMI 硬件显示开关 (`0`: 关, `1`: 开)<br><br>  <br>  <br><br>- `rtsp-port`: RTSP 推流端口 (默认 8554)<br><br>  <br>  <br><br>- `venc_json`: 指定编码参数 json 文件路径|
|**`vc_param.json`**|**VENC 编码及 RTSP 属性文件**<br><br>  <br>  <br><br>控制视频编码器参数及网络推流品质。|- `codec`: 编码格式 (`265` 即 H.265, `264` 即 H.264)<br><br>  <br>  <br><br>- `bitrate`: 目标码率 (如 4096 / 5000 kbps)<br><br>  <br>  <br><br>- `gop`: 关键帧间隔 (默认 50)<br><br>  <br>  <br><br>- `SrcFrmRate` / `DstFrmRate`: 输入/输出帧率 (默认 25 fps)|
|**`CviIspTool.sh`**|**程序启动环境配置脚本**<br><br>  <br>  <br><br>配置 Linux 内核网络 Socket 缓存区（保障高码率推流不卡顿），并调用后台核心引擎。|- 优化 `/proc/sys/net/core/wmem_max`<br><br>  <br>  <br><br>- 启动底层核心 `isp_tool_daemon`|

### 3. ISP Demo 数据流架构原理

整个相机系统的图像数据处理与分流流向如下：

  

Plaintext

```
  [ Camera Sensor (GC4653) ]
             │  (MIPI CSI 接口传输 Raw 图像)
             ▼
      [ VI 子系统 & ISP ]  ───> (AE 自动曝光 / AWB 白平衡 / 3D降噪 处理)
             │  (输出 NV21/NV12 格式 YUV 图像)
             ├──────────────────────────────────────────┐
             │ (当 enable-hmdi: 1 时)                    │ (默认并行流程)
             ▼                                          ▼
   [ VO 子系统 (Video Out) ]                     [ VENC 编码子系统 ]
             │                                          │
             ▼                                          ▼
      [ HDMI 显示输出 ]                          [ H.265/H.264 编码器 ]
       (显示器实时画面)                                   │
                                                        ▼
                                                 [ RTSP 推流服务 ]
                                               (8554 端口 / 网络拉流)
```

**流转步骤拆解**：

  

1. **采样与传输**：GC4653 摄像头通过 MIPI CSI 通道将原始 RAW10 图像数据送入板载 ISP。
    
      
    
2. **ISP 算法打磨**：`isp_tool_daemon` 驱动 ISP 硬件对 RAW 图像进行丢帧对焦、降噪、颜色校正，并转换为通用 YUV (NV21) 格式。
    
      
    
3. **分流处理**：
    
      
    - **HDMI 管道（VO 子系统）**：开启 `enable-hmdi: 1` 后，ISP 吐出的帧数据直接提交给 VO (Video Out) 模块，无需占用 CPU 即可通过硬件显示控制器送至 HDMI 屏。
        
          
        
    - **网络推流管道（VENC 子系统）**：同步将帧送入 VENC 模块按 `vc_param.json` 参数压缩为 H.265 码流，通过 RTSP 协议暴露给局域网播放器（如 VLC）拉流。