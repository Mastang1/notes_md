根据《EB tresos® Studio 开发手册》第 6.2.4 章节关于 GUI 元素的说明，以下是一个包含所有核心控件、容器及 Tab 页配置示例的 `demo.xdm` 文档结构。

==最好直接阅读 NXP 工程XDM==

### demo.xdm 示例文件

```
<?xml version='1.0'?>
<datamodel version="7.0"
    xmlns="http://www.tresos.de/_projects/DataModel2/16/root.xsd"
    xmlns:a="http://www.tresos.de/_projects/DataModel2/16/attribute.xsd"
    xmlns:v="http://www.tresos.de/_projects/DataModel2/06/schema.xsd"
    xmlns:d="http://www.tresos.de/_projects/DataModel2/06/data.xsd">

  <v:ctr name="DemoModule" type="MODULE-DEF">
    <a:a name="DESC" value="这是一个包含所有常用 GUI 元素的 Demo 模块"/>

    <!-- 1. 基础变量控件 (文本框、勾选框等) -->
    <v:ctr name="BasicWidgets" type="IDENTIFIABLE">
      <a:a name="TAB" value="基础控件页"/> <!-- 定义 Tab 页名称 -->

      <!-- 布尔值：显示为复选框 -->
      <v:var name="MyBoolean" type="BOOLEAN">
        <a:a name="LABEL" value="启用功能 (Boolean)"/>
        <a:da name="TOOLTIP" value="勾选以启用示例功能"/>
      </v:var>

      <!-- 整数：显示为文本框，可设置进制 -->
      <v:var name="MyInteger" type="INTEGER">
        <a:a name="LABEL" value="计数器 (Integer)"/>
        <a:a name="DEFAULT_RADIX" value="HEX"/> <!-- 默认显示为十六进制 -->
      </v:var>

      <!-- 枚举：显示为下拉组合框 -->
      <v:var name="MyEnum" type="ENUMERATION">
        <a:a name="LABEL" value="工作模式 (Enumeration)"/>
        <a:da name="RANGE">
          <a:v>NORMAL</a:v>
          <a:v>POLLING</a:v>
        </a:da>
      </v:var>
    </v:ctr>

    <!-- 2. 容器示例：支持折叠和布局控制 -->
    <v:ctr name="ContainerExample" type="IDENTIFIABLE">
      <a:a name="TAB" value="布局示例页"/>
      <a:da name="FRAME" value="TITLE"/> <!-- 带标题的可折叠框架 -->
      <a:a name="LAYOUT" value="HORIZONTAL"/> <!-- 内部控件水平布局 -->

      <v:var name="Horiz1" type="STRING"><a:a name="LABEL" value="水平变量1"/></v:var>
      <v:var name="Horiz2" type="STRING"><a:a name="LABEL" value="水平变量2"/></v:var>
    </v:ctr>

    <!-- 3. 选择器 (Choice)：显示为下拉框加子 Tab 页 -->
    <v:chc name="MyChoice" type="IDENTIFIABLE">
      <a:a name="TAB" value="选择器页"/>
      <v:ctr name="Option_A" type="IDENTIFIABLE">
        <v:var name="VarA" type="INTEGER"/>
      </v:ctr>
      <v:ctr name="Option_B" type="IDENTIFIABLE">
        <v:var name="VarB" type="STRING"/>
      </v:ctr>
    </v:chc>

    <!-- 4. 列表/表格 (List)：默认生成独立 Tab 页，展示为表格 -->
    <v:lst name="MyTable" type="MAP">
      <a:a name="TAB" value="表格展示页"/>
      <a:a name="COLUMNS"> <!-- 定义表格列 -->
        <a:v>ColumnString</a:v>
        <a:v>ColumnInt</a:v>
      </a:a>
      <v:ctr name="TableRow" type="IDENTIFIABLE">
        <v:var name="ColumnString" type="STRING">
          <a:a name="COLUMN_TITLE" value="字符串列"/>
        </v:var>
        <v:var name="ColumnInt" type="INTEGER">
          <a:a name="COLUMN_TITLE" value="整数列"/>
        </v:var>
      </v:ctr>
    </v:lst>

  </v:ctr>
</datamodel>
```

### 关键配置点解析：

1. **Tab 页创建**：通过在容器或控件上使用 `<a:a name="TAB" value="页名称"/>` 来手动将元素分配到特定的选项卡。
2. **容器 (Container)**：使用 `v:ctr` 定义。通过 `FRAME` 属性可以控制其显示样式（如 `LINE` 线框或 `TITLE` 带标题的可折叠框）。
3. **控件类型**：`v:var` 的 `type` 属性决定了它是 Checkbox (`BOOLEAN`) 还是文本框 (`INTEGER`/`STRING`)。
4. **表格 (List)**：`v:lst` 且 `type="MAP"` 会被渲染为表格。通过 `COLUMNS` 属性指定哪些子参数作为列显示。

您是否需要我为您进一步演示如何使用 **XPath 自动属性**（如 `VISIBLE`）来动态隐藏这些控件？