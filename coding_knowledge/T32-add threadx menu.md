# T32 配置 ARM ThreadX 调试菜单指南

## 📂 核心文件与路径
* **启动脚本文件**：`system-settings.cmm` (或 `autostart.cmm`)
* **启动脚本路径**：T32 安装根目录 (例如 `C:\T32\`，在 CMM 脚本中表示为 `~~/`)
* **ThreadX 内核支持路径**：`~~/demo/arm/kernel/threadx/`
* **ThreadX 内核解析引擎**：`threadx.t32`
* **ThreadX 图形菜单文件**：`threadx.men`

---

## 🛠️ 操作配置流程

1. 使用文本编辑器打开 T32 安装根目录下的 **`system-settings.cmm`** 文件。
2. 滚动至文件尾部，在 `ENDDO` 或 `END` 命令之前，粘贴以下配置代码：

```shell
; =========================================================
; 注入自定义配置：加载 ARM ThreadX RTOS 感知支持
; =========================================================

LOCAL &ThreadX_Base
&ThreadX_Base="~~/demo/arm/kernel/threadx"

IF OS.FILE("&ThreadX_Base/threadx.t32")
(
    ; 加载 ThreadX 数据结构解析配置
    TASK.CONFIG "&ThreadX_Base/threadx.t32"
    
    ; 增量添加 ThreadX 专属图形化下拉菜单
    MENU.ADD "&ThreadX_Base/threadx.men"
    
    PRINT "ThreadX OS Awareness for ARM loaded successfully."
)
ELSE
(
    PRINT "Error: Cannot find ThreadX scripts at &ThreadX_Base"
)