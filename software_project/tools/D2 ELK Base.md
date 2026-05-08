这是一份专为开发者整理的 **D2 + ELK 工业级绘图部署与实战指南**。它将理论浓缩到最简，并提供了一条开箱即用的自动化工作流。

---

# 架构师备忘录：D2 + ELK 工业级流程图指南

## 1. 降维双擎：简介与简史

在代码可视化领域，D2 和 ELK 是“前端语法”与“后端算法”的最强组合。

### ELK (Eclipse Layout Kernel)：极客的数学引擎

- **身世**：起源于德国基尔大学（Kiel University）的研究，后被贡献给 Eclipse 基金会。
    
- **初衷**：它最早根本不是为了画软件图，而是为了**硬件 EDA（电子设计自动化）**，用来绘制复杂的集成电路网表和 PCB 电路板。
    
- **降维打击**：硬件布线不允许斜线交叉和重叠。ELK 将这种正交寻路算法（Orthogonal Routing）降维应用到软件架构图中，从数学层面上彻底消灭了 Mermaid 和 Graphviz 中常见的“线段乱飞”、“莫名穿模”问题。
    

### D2 (Declarative Diagramming)：现代化的翻译官

- **身世**：由 Terrastruct 公司在 2022 年开源的现代声明式绘图语言。
    
- **初衷**：取代语法反人类、年代久远的 Graphviz (DOT) 和排版混乱的 Mermaid。
    
- **绝配**：D2 的语法像 JSON/CSS 一样干净，且**官方编译器底层原生集成了 ELK 引擎**。你只需写人话，D2 负责喂给 ELK 算出完美坐标。
    

---

## 2. 本地极速部署流程 (3 分钟)

与 PlantUML 需要配置 Java 和 Graphviz 环境不同，D2 是用 Go 语言编写的，**单文件绿色免安装，且内置了 ELK**。

### 步骤 1：安装 D2 命令行工具 (CLI)

打开终端，根据你的操作系统执行：

- **Windows (推荐使用 Winget 或 Scoop)**:
    
    PowerShell
    
    ```
    winget install Terrastruct.D2
    # 或者
    scoop install d2
    ```
    
- **macOS**:
    
    Bash
    
    ```
    brew install d2
    ```
    
- **Linux**:
    
    Bash
    
    ```
    curl -fsSL https://d2lang.com/install.sh | sh -s --
    ```
    

### 步骤 2：安装 Cursor / VS Code 插件

1. 在 Cursor 扩展商店搜索 **`D2`**（认准作者为 Terrastruct）。
    
2. 安装后，它提供语法高亮、实时预览和悬浮提示。
    

### 步骤 3：验证与实时渲染

新建一个 `test.d2` 文件，输入 `x -> y`。

在终端运行实时监听命令：

Bash

```
d2 --layout=elk --watch test.d2 test.svg
```

_这会在浏览器中打开一个本地网页，你每次保存代码，页面都会瞬间刷新出由 ELK 引擎排版的高清 SVG。_

---

## 3. 程序员实战：Code 转流程图的最优解

**核心理念：** 不要手写 D2，让大模型（LLM）做 AST（抽象语法树）解析，直接输出 D2 代码。

### 第一步：将系统级 Prompt 注入 Cursor

在你的项目根目录创建或更新 `.cursorrules` 文件，强控 AI 的输出规范：

Markdown

```
# 流程图生成协议
当你需要将代码逻辑可视化时，必须使用 D2 语言，并严格遵守以下规则：
1. **排版引擎**：必须在首行声明 `direction: down`。
2. **节点定义**：分离定义与连接。先统一定义判断节点（shape: diamond）和执行节点（shape: rectangle）。
3. **连线约束**：使用大写首字母的标语（如 `: Yes`, `: No`）。
4. **错误处理**：异常退出的节点必须加上样式 `style: { fill: "#ffebee" }`。
```

### 第二步：一键生成与渲染示例

当你在 Cursor 中框选一段 C 语言函数并让 AI 生成图表时，它会吐出如下干净的 D2 代码：

代码段

```
# 全局方向向下
direction: down

# 预定义节点与样式
Start: ipcsBufPoolInit { shape: oval; style.fill: "#e3f2fd" }
CheckNum: "num_bufs <= MAX?" { shape: diamond; style.fill: "#fff9c4" }
SetParams: Set pool->num_bufs
ErrorExit: return err { shape: oval; style.fill: "#ffebee" }

# 逻辑连线 (ELK 引擎会自动计算正交路径)
Start -> CheckNum
CheckNum -> SetParams: Yes
CheckNum -> ErrorExit: No
SetParams -> ...
```

### 第三步：输出交付物

在终端执行编译命令，强制调用 ELK 引擎：

Bash

```
d2 --layout=elk architecture.d2 architecture.png
```

---

这套方案抛弃了所有历史包袱，用最新的工具链实现了最高规格的排版。你目前的项目中，是否有包含复杂嵌套循环或状态机的代码块，需要我立刻用 D2 语法为你生成一个演示案例？