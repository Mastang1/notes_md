在 EB tresos 的 XDM 格式中，属性是为数据模型（DataModel）节点添加元数据的核心手段。`a:da` 与 `a:a` 的核心区别在于**值的确定方式**以及**对 UI 交互的动态控制力**。

### 1. `a:da` (Automatic Attributes) 的作用

`a:da` 代表“自动属性”或“动态属性”。

- **计算机制**：它们不存储固定值，而是通过查询 DataModel 的配置来**动态计算**自身的值。
- **核心功能**：主要用于实现参数间的逻辑联动。例如，根据一个参数的取值来决定另一个参数是否有效、可见或合法。
- **常见类型**：包括 `XPath`（最强但也最慢）、`Range`（范围检查）、`Match`（值匹配）和 `Bound`（节点绑定）等。

### 2. `a:da` 与 `a:a` 的对比分析

|特性|`a:a` (Passive/Generic Attributes)|`a:da` (Automatic Attributes)|
|:--|:--|:--|
|**数值性质**|**静态/被动**：值在 XDM 中显式定义。|**动态/自动**：值通过测试表达式（`expr`）计算得出。|
|**存储内容**|用于存储固定文本（如 `LABEL`）或复杂数据结构（如 `ADMIN-DATA`）。|用于存储校验逻辑、联动规则或计算公式。|
|**继承性**|通常定义在当前节点。|作为“数据属性”定义在模式节点（Schema）时，会被绑定的数据节点（Data）继承。|

### 3. 对 UI 的具体影响

`a:da` 的引入使得 EB tresos 的图形界面从“静态表单”转变为“智能交互界面”：

- **动态可见性与可用性**：
    - `a:a` 定义静态标签（`LABEL`），界面文字固定。
    - `a:da` 控制 `VISIBLE` 和 `EDITABLE`。例如，当 `LinNodeType` 为 `MASTER` 时，通过 `a:da` 动态显示 Master 特有的配置项，否则隐藏或置灰。
- **实时校验提醒**：
    - 通过 `a:da` 的 `INVALID` 或 `WARNING` 类型，UI 会根据用户输入实时生成错误或警告图标。例如，若 `LinChannelId` 重复，UI 会立即显示报错信息。
- **自动化辅助功能**：
    - **计算按钮**：若为参数定义了 `a:da` 类型的 `DEFAULT` 属性，UI 会在输入框旁显示一个 **“Calculate” (计算)** 按钮，允许用户点击以根据当前逻辑重新填充默认值。
- **进制转换**：
    - 虽然 `DEFAULT_RADIX`（默认进制）通常是静态的（`a:a`），但 `a:da` 可以决定数值在不同条件下的显示范围（`RANGE`）。

**Demo 示例（结合代码上下文）**： 在 `Lin_43_LLCE.xdm` 中，`LinChannelId` 使用了 `a:da` 进行唯一性校验：

```
<v:var name="LinChannelId" type="INTEGER">
    <a:da name="INVALID" type="XPath"
          expr="node:uniq(../../*/LinChannelId, .)"
          false="Duplicated value..."/> <!-- 动态校验 UI 报错 -->
</v:var>
```

相比之下，`LABEL` 始终使用 `a:a`，因为它在界面上是不可改变的静态文本。

你是否想进一步了解如何编写 `a:da` 中的 **XPath 表达式** 来实现复杂的跨模块参数校验？