
这是一份经过深度提炼的“PlantUML 源码自动化可视化笔记”。它整合了我们在会话中解决的所有核心痛点（菱形显示、版本兼容、自动化构建、矢量集成），旨在为你提供一套开箱即用的工业级部署方案。

---

# 首席架构师笔记：PlantUML 源码自动化可视化全攻略

## 1. 核心选型结论：为什么是 PlantUML？

在对比了 Mermaid、Graphviz (DOT) 和 D2 后，针对 **C 语言底层函数逻辑（if/else/while/return）**，PlantUML 是唯一的工业级最优解。

- **语义对齐**：它拥有专为程序控制流设计的 `if/while/switch` 语法，而非通用的有向无环图（DAG）。
    
- **布局稳定**：原生支持正交布线（直角）和逻辑切断（detach），消灭了线段乱飞和断线问题。
    
- **工程化强**：完美支持 SVG 矢量图，适合嵌入 Word/PDF 交付文档。
    

---

## 2. 环境部署与兼容性修复（避坑指南）

### 2.1 基础依赖

1. **Java**：JRE 11+（运行 `java -version` 确认）。
    
2. **Graphviz**：下载安装并勾选“添加到系统路径”。
    
3. **环境变量**：必须手动添加系统变量 `GRAPHVIZ_DOT`，值指向 `...\bin\dot.exe`。
    

### 2.2 版本代差修复（菱形变小/乱码的关键）

- **痛点**：旧版 PlantUML (2021) 无法识别新版 Graphviz (2026) 的版本号，导致渲染降级（菱形变小点或六边形）。
    
- **方案**：前往 [PlantUML 官网](https://plantuml.com/download) 下载最新的 `plantuml-1.2026.x.jar`。
    
- **Cursor 配置**：在设置中搜索 `Plantuml: Jar`，将路径指向下载好的新包。执行 `testdot` 确认 `Warning` 消失。
    

---

## 3. 核心语法规范（保证格式整洁）

为了实现“菱形等大、文字居中、主干笔直、右侧异常”，必须在 `.puml` 头部注入以下强控配置：

代码段

```
@startuml
' --- 环境强控 ---
!pragma layout smetana        ' 强制使用内置 Java 引擎优化排版
skinparam linetype ortho      ' 强制直角连线
skinparam conditionStyle insideDiamond ' 强制文字进入菱形并撑大尺寸

' --- 视觉规范 ---
skinparam ActivityDiamondPadding 10
skinparam ActivityDiamondBackgroundColor #fff9c4
skinparam ActivityBackgroundColor #f8f9fa
skinparam DefaultFontName Consolas

' --- 语法示例 ---
start
if (判断条件?) then (Yes)
    :主干动作;
else (No)
    #ffebee:错误处理;
    detach                     ' 核心：切断连线，避免线段绕回
endif
#fbe9e7:return err;
stop
@enduml
```

---

## 4. 自动化构建与文档集成

### 4.1 自动化渲染命令 (CLI)

- **生成高清 PNG**：`java -jar plantuml.jar -tpng flow.puml`
    
- **生成矢量 SVG (推荐)**：`java -jar plantuml.jar -tsvg flow.puml`
    
- **实时监控模式**：`java -jar plantuml.jar -watch .`（文件夹内文件一保存，图片立即更新）
    

### 4.2 SVG 与 Word 集成

- **优势**：SVG 在 Word 中无限放大不模糊，支持 Office 365 / 2019+。
    
- **操作**：直接拖拽 `.svg` 到 Word 即可。
    
- **旧版兼容**：若 Office 版本过低，使用 `-temf` 导出 Windows 矢量元文件。
    

---

## 5. AI Agent (Cursor) 协同规则

为了让 AI 以后全自动产出符合上述规范的代码，请将以下指令加入你的 **`.cursorrules`**：

> **[Workflow: C-Code-to-Graph]**
> 
> 1. 禁用 Mermaid。仅允许输出 **PlantUML V2 活动图** 语法。
>     
> 2. 必须包含 `!pragma layout smetana` 和 `insideDiamond` 配置。
>     
> 3. 判断语句必须采用 `if (label) then (...)` 格式，确保文字被菱形包裹。
>     
> 4. 异常退出（return/error）必须分配颜色 `#ffebee` 并紧跟 `detach` 关键字。
>     
> 5. 所有的连线必须使用 `linetype ortho` 保持直角。
>     

---

## 💡 记忆口诀

- **装两个**：Java 和最新版 Graphviz。
    
- **换一个**：换掉插件自带的旧版 `plantuml.jar`。
    
- **设两个**：`GRAPHVIZ_DOT` 变量和 `insideDiamond` 样式。
    
- **记一个**：`detach` 解决所有连线混乱。
    

这套笔记可以作为你团队内部的“源码可视化标准规范”，确保所有生成的架构文档具备一致的、高水准的工程美感。