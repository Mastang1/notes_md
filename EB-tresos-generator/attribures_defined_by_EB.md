# 1. Reference chapter 5.1.4.3. Automatic attributes

根据 EB tresos 开发指南第 5.1.4.3 章节的内容，自动属性（Automatic attributes）通过查询 DataModel 中的配置来动态计算数值，每当属性值被请求时都会重新计算。

以下是七种自动属性的 Markdown 格式 Demo：

### 1. Bound (绑定属性)

直接引用树中另一个节点的值。常用于将一个参数的默认值与另一个参数同步。

```
<v:var name="Int1" type="INTEGER"/>
<v:var name="Int2" type="INTEGER">
  <!-- Int2 的默认值将始终等于 Int1 的当前值 -->
  <a:da name="DEFAULT" type="Bound" expr="../Int1"/>
</v:var>
```

### 2. Match (匹配属性)

将指定节点的 XPath 值与固定值进行比较。

```
<a:da name="TITLE" type="Match">
  <!-- 如果 Var1 等于 3，显示 true 指定的文本，否则显示 false 指定的文本 -->
  <a:tst expr="//Var1 = 3"
         true="不要输入内容"
         false="请输入内容"/>
</a:da>
```

### 3. Range (范围属性)

用于定义数值的范围检查。如果值不在范围内，工具会生成对应的错误消息。

```
<a:da name="INVALID" type="Range">
  <a:tst expr=">=i(-15)"/>
  <a:tst expr="<=i(30)"/>
  <a:tst expr="!=0"/>
  <!-- 合法范围为 [-15, 30]，但排除 0 -->
</a:da>
```

### 4. XPath (XPath 表达式属性)

使用完整的 XPath 语言进行评估，功能最强但执行速度相对较慢。

```
<!-- 仅当 Integer2 的值小于 20 时，当前节点才可编辑 -->
<a:da name="EDITABLE" type="XPath" expr="../Integer2 &lt; 20"/>
```

### 5. Multi (多重测试属性)

允许在一个属性中混合使用多种查询语言（如长度、正则、范围等）。

```
<a:a name="INVALID" type="Multi">
  <mt:length><mt:tst expr=">=6"/></mt:length> <!-- 检查字符串长度 -->
  <mt:range><mt:tst expr="&lt;=255"/><mt:tst expr="&gt;=0"/></mt:range> <!-- 检查数值范围 -->
  <mt:xpath expr=". &gt;= 42"/> <!-- 额外的 XPath 校验 -->
</a:a>
```

### 6. EcucCond (ECUC 条件属性)

**仅限**与 `ENABLE` 数据属性结合使用，基于 AUTOSAR 的条件公式进行评估。

```
<a:da name="ENABLE" type="EcucCond">
  <f:formulaExpr>
    <f:formula>
      reference("ECUC-QUERY:/path/to/GetTTCanEnabled")
    </f:formula>
  </f:formulaExpr>
</a:da>
```

### 7. EcucValidCond (ECUC 校验条件属性)

**仅限**与 `INVALID` 数据属性结合使用，定义符合 AUTOSAR 规范的有效性约束。

```
<a:da name="INVALID" type="EcucValidCond">
  <f:formulaExpr name="ecucValidCond">
    <f:formula>
      reference("ECUC-QUERY:/path/to/GetTTCanEnabled")
    </f:formula>
  </f:formulaExpr>
</a:da>
```

**注意事项**：

- 自动属性不能访问配置类（Configuration Class）晚于自身的节点值。
- 带 `DEFAULT` 自动属性的节点在 GUI 中会显示一个“计算 (Calculate)”按钮，供用户手动触发重新计算。

这些 Demo 是否涵盖了你目前开发中需要的场景？如果需要针对 `Lin` 模块编写特定的 `RANGE` 校验逻辑，我可以为你提供更具体的 XPath 例子。



# 2. Refer to: chapter 5.1.4.3.8. XDM attribute examples