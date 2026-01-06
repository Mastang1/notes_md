以下是根据《EB tresos® Studio 开发手册》第 6.2.4 章节总结的 **GUI 元素配置笔记**。

---

# EB tresos GUI 元素配置笔记 (XDM)

在 EB tresos 中，通用配置编辑器（Generic Configuration Editor）会根据 XDM 文件中的参数定义自动生成图形界面。开发者可以通过添加特定的 **GUI 注释（GUI Annotation）** 来精确控制界面的呈现方式。

## 1. 核心属性概览 (Attributes)

GUI 元素的功能主要通过属性实现。属性分为静态属性（使用 `<a:a>` 标签）和动态自动属性（使用 `<a:da>` 标签，支持 XPath 表达式）。

|属性名称|适用范围|功能描述|
|:--|:--|:--|
|**LABEL**|所有元素|修改界面显示的标签名称（`<a:a>` 静态属性）。|
|**TOOLTIP**|所有元素|为控件添加悬停提示信息。|
|**VISIBLE**|所有元素|控制控件是否可见，支持动态隐藏。|
|**EDITABLE**|变量/容器|控制控件是否可编辑。若为 `false`，控件变灰。|
|**DESC**|所有元素|在 "Description" 视图中显示详细说明，支持 HTML 和超链接。|
|**TAB**|所有元素|手动将元素分配到指定的选项卡（Tab）页中。|
|**WIDTH**|文本框/下拉框|定义输入框的字符宽度。|

---

## 2. 常用 GUI 控件示例

### 2.1 布尔值 (Boolean Values)

布尔型变量在编辑器中呈现为 **复选框 (Checkbox)**。

- **配置示例：**
    
    ```
    <v:var name="MyBoolean" type="BOOLEAN">
        <a:a name="LABEL" value="是否启用功能"/>
        <a:da name="TOOLTIP" value="勾选此项以激活模块"/>
    </v:var>
    ```
    
- **效果：** --Image of: --复选框示例 _(Figure 6.12: Check box)_

### 2.2 基础字段 (Integer, Float, String, Enum)

- **文本框 (Text Field)：** 用于整数、浮点数和字符串。
- **组合框 (Combo Box)：** 用于枚举（Enumeration）和引用（Reference）。
- **进制控制 (DEFAULT_RADIX)：** 仅限整数，可设置为 `DEC` (十进制)、`HEX` (十六进制)、`OCT` (八进制) 或 `BIN` (二进制)。
    - **效果：** --Image of: --进制示例 _(Figure 6.27: HEX representation)_

---

## 3. 容器与布局 (Container)

容器用于对参数进行分组。

- **框架样式 (FRAME)：** 支持 `NONE` (无边框)、`LINE` (有标题线框) 或 `TITLE` (带标题且可折叠，默认值)。
- **排列方式 (LAYOUT)：** 支持 `VERTICAL` (垂直，默认) 或 `HORIZONTAL` (水平)。
- **效果：** --Image of: --垂直布局 _(Figure 6.35: Vertical Layout)_

---

## 4. 选择器 (Choices)

选择器元素会生成一个下拉列表，用户选择不同的选项会激活对应的子容器（以 Tab 页签形式展示）。

- **特点：** 只有当前选中的子 Tab 中的参数是生效且可编辑的。
- **效果：** --Image of: --选择器 _(Figure 6.38: Choice element)_

---

## 5. 列表与表格 (Lists)

当列表类型设为 `MAP` 时，它在界面中以 **表格 (Table)** 形式展示。

- **COLUMNS 属性：** 定义表格中显示哪些子参数作为列。
- **COLUMN_TITLE：** 定义列标题。
- **效果：** --Image of: --表格示例 _(Figure 6.49: List as Table)_

---

## 6. 进阶特性

- **自动分栏 (Tab Assignment)：** 当一个选项卡下的元素超过 100 个时，系统会自动将后续容器移至新选项卡，以保证性能。
- **超链接导航：** 可以在 `DESC` 属性中使用 HTML 标签创建内部链接，点击后可跳转至其他配置节点（格式：`<a href="path:/XPath/To/Node">`）。

---

**下一步建议：** 您是否想尝试编写一个复杂的 **XPath 自动属性**？例如：根据一个 Checkbox 的状态来动态隐藏（`VISIBLE`）整个容器？