根据您上传的文档，以下是 **第 91-100 页** 的详细翻译。

这一部分继续深入讲解了 **AUTOSAR 数据类型到 XDM Data 节点的映射**，包括各种基本类型（布尔、整数、浮点等）、引用类型、属性映射，以及列表节点和选择节点的特殊处理规则。

---

### 5.2.1.14. Data 节点 (Data-Nodes) (续)

#### 5.2.1.14.5. BOOLEAN-VALUE (布尔值)

对应的 Schema 节点：见 5.2.1.1.5 节 "BOOLEAN-PARAM-DEF"。

* **简单形式 (Simple):**
```xml
<d:var name="MyBoolean" type="BOOLEAN" value="true"/>

```


* **带有外围列表的形式 (With surrounding list):**
```xml
<d:lst name="MyBoolean">
  <d:var type="BOOLEAN" value="true"/>
  <d:var type="BOOLEAN" value="false"/>
</d:lst>

```



#### 5.2.1.14.6. INTEGER-VALUE (整数值)

对应的 Schema 节点：见 5.2.1.1.6 节 "INTEGER-PARAM-DEF"。

* **简单形式:**
```xml
<d:var name="MyInteger" type="INTEGER" value="12"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="MyInteger">
  <d:var type="INTEGER" value="10"/>
  <d:var type="INTEGER" value="11"/>
</d:lst>

```



#### 5.2.1.14.7. FLOAT-VALUE (浮点值)

对应的 Schema 节点：见 5.2.1.1.7 节 "FLOAT-PARAM-DEF"。

* **简单形式:**
```xml
<d:var name="MyFloat" type="FLOAT" value="1.0"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="MyFloat">
  <d:var type="FLOAT" value="1.0"/>
  <d:var type="FLOAT" value="2.0"/>
</d:lst>

```



#### 5.2.1.14.8. STRING-VALUE (字符串值)

对应的 Schema 节点：见 5.2.1.1.8 节 "STRING-PARAM-DEF"。

* **简单形式:**
```xml
<d:var name="MyString" type="STRING" value="String"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="MyString">
  <d:var type="STRING" value="String1"/>
  <d:var type="STRING" value="String2"/>
</d:lst>

```



#### 5.2.1.14.9. LINKER-SYMBOL-VALUE (链接器符号值)

对应的 Schema 节点：见 5.2.1.1.9 节 "LINKER-SYMBOL-DEF"。

* **简单形式:**
```xml
<d:var name="MyLinkerSymbol" type="LINKER-SYMBOL" value="Symbol"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="MyLinkerSymbol">
  <d:var type="LINKER-SYMBOL" value="Symbol1"/>
  <d:var type="LINKER-SYMBOL" value="Symbol2"/>
</d:lst>

```



#### 5.2.1.14.10. FUNCTION-NAME-VALUE (函数名值)

对应的 Schema 节点：见 5.2.1.1.10 节 "FUNCTION-NAME-DEF"。

* **简单形式:**
```xml
<d:var name="MyFunctionName" type="FUNCTION-NAME" value="Function"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="MyFunctionName">
  <d:var type="FUNCTION-NAME" value="Func1"/>
  <d:var type="FUNCTION-NAME" value="Func2"/>
</d:lst>

```



#### 5.2.1.14.11. ENUMERATION-VALUE (枚举值)

对应的 Schema 节点：见 5.2.1.1.11 节 "ENUMERATION-PARAM-DEF"。

* **简单形式:**
```xml
<d:var name="MyEnum" type="ENUMERATION" value="Enum1"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="MyEnum">
  <d:var type="ENUMERATION" value="Enum1"/>
  <d:var type="ENUMERATION" value="Enum2"/>
</d:lst>

```



#### 5.2.1.14.12. REFERENCE-VALUE (引用值)

包括：`REFERENCE-VALUE`, `CHOICE-REFERENCE-VALUE`, `SYMBOLIC-NAME-REFERENCE-VALUE`, `FOREIGN-REFERENCE-VALUE`, `ECUC-URI-REFERENCE-VALUE`。

对应的 Schema 节点：见 5.2.1.3, 5.2.1.4, 5.2.1.5, 5.2.1.7, 5.2.1.8 节。

* **简单形式:**
```xml
<d:ref name="myRef" value="ASPath:/..." type="REFERENCE"/>

```


* **带有外围列表的形式:**
```xml
<d:lst name="myRef">
  <d:ref name="myRef1" value="ASPath:/..." type="REFERENCE"/>
  <d:ref name="myRef2" value="ASPath:/..." type="REFERENCE"/>
</d:lst>

```



#### 5.2.1.14.13. INSTANCE-REFERENCE-VALUE (实例引用值)

对应的 Schema 节点：见 5.2.1.6 节 "INSTANCE-REFERENCE-DEF"。

* **简单形式:**
```xml
<d:ctr name="iRef" type="INSTANCE">
  <d:ref name="TARGET" value="ASPath:/..." type="REFERENCE"/>
  <d:lst name="CONTEXT">
    <d:ref value="ASPath:/..." type="REFERENCE"/>
    <d:ref value="ASPath:/..." type="REFERENCE"/>
  </d:lst>
</d:ctr>

```


* **带有外围列表的形式:**
```xml
<d:lst name="iRef">
  <d:ctr name="iRef1" type="INSTANCE">
    <d:ref name="TARGET" value="ASPath:/..." type="REFERENCE"/>
    <d:lst name="CONTEXT">
      <d:ref value="ASPath:/..." type="REFERENCE"/>
    </d:lst>
  </d:ctr>
</d:lst>

```



#### 5.2.1.14.14. 示例

*(此处文档引用了图 5.2 和 图 5.3，展示了 Schema 和 Data 节点的层次结构，说明了 Data 节点如何引用 Schema 节点)*

---

### 5.2.2. 属性映射 (Attribute Mapping)

Schema 节点的属性直接映射到 Data 节点的属性。

#### 5.2.2.1. CONFIGURATION-CLASS-AFFECTION

在 AUTOSAR 2.0/2.1 中，`CONFIGURATION-CLASS-AFFECTION` 定义了一个参数在不同配置变体（PreCompile, LinkTime, PostBuild）下的可见性。

在 XDM 中，这被映射到 Schema 节点的一个名为 `IMPLEMENTATIONCONFIGCLASS` 的通用属性：

```xml
<a:a name="IMPLEMENTATIONCONFIGCLASS" type="IMPLEMENTATIONCONFIGCLASS">
  <icc:v class="PreCompile">VariantPreCompile</icc:v>
  <icc:v class="Link">VariantLinkTime</icc:v>
  <icc:v class="PostBuild">VariantPostBuild</icc:v>
</a:a>

```

此属性定义了该节点在特定配置变体（Variant）下属于哪个配置类（PreCompile, LinkTime, PostBuild）。

---

### 5.2.3. 使用 ECU-PD 进行 ECU-CD 的 XPath 寻址 (XPath-addressing of ECU-CD using ECU-PD)

在 DataModel 中，Data 节点通常指向定义它的 Schema 节点。但是，Data 节点没有直接指向 AUTOSAR ECU 参数定义（ECU Parameter Definition, ECU-PD）的链接。

为了通过 XPath 访问 ECU-PD 信息（例如获取参数的默认值或范围），可以使用特殊的寻址方式。

#### 5.2.3.1. 模板中的路径 (Path in Template)

在代码生成模板中，可以使用特定的 XPath 函数来访问定义信息。

* **as:dtos(path)**: Data TO Schema。从 Data 节点路径获取对应的 Schema 节点。
* **as:stod(path)**: Schema TO Data。从 Schema 节点路径获取所有引用该定义的 Data 节点。

*(文档强调了 XDM 结构中 Schema 与 Data 的分离但对应的关系，并提供了在生成代码时如何通过 Data 节点反查 Schema 定义的方法)*

---

### 5.2.4. 顶层结构 (Top level Structure)

DataModel 的顶层结构反映了 AUTOSAR 的包结构（AR-PACKAGE）。

* **AR-PACKAGE** 映射为 `d:ctr type="AR-PACKAGE"`。
* **MODULE-CONFIGURATION** 映射为 `d:ctr type="MODULE-CONFIGURATION"`。

在 XDM 中，顶层通常包含一个或多个 `AR-PACKAGE`，其中包含模块配置。

---

### 5.2.5. 列表节点 (List-Nodes)

在 AUTOSAR 中，当参数的 `UPPER-MULTIPLICITY` 大于 1 时，表示该参数可以出现多次。在 XDM 中，这通过列表节点（`v:lst` / `d:lst`）来表示。

* **Schema 树**：列表节点 (`v:lst`) 包含**一个**子节点，作为列表项的模板。
* **Data 树**：列表节点 (`d:lst`) 包含**多个**子节点，每个子节点都是基于模板创建的实例。

#### 5.2.5.1. 可选元素 (Optional elements)

AUTOSAR 可以定义“可选”元素，即 `LOWER-MULTIPLICITY = 0` 且 `UPPER-MULTIPLICITY = 1`。

在 DataModel 中，这有两种表示方式：

1. **旧方式（不推荐）- 包装在列表中**：
将可选元素包装在一个 `v:lst` 中，并设置 `MIN=0`, `MAX=1`。
* **缺点**：GUI 会为每个元素显示单独的标签页；XPath 路径会发生变化（需要检查列表是否存在或列表项是否存在）。


2. **新方式（推荐）- 使用 OPTIONAL 属性**：
直接标记元素为可选，而不将其包装在列表中。
```xml
<v:var name="MyOptionalInteger" type="INTEGER">
  <a:a name="OPTIONAL" value="true"/>
  <a:da name="ENABLE" value="false"/>
</v:var>

```


* **优点**：XPath 路径保持一致，不依赖于元素是否被启用。可以通过检查 `node:exists(ElementName)` 来判断。



#### 5.2.5.2. 命令行中的可选元素

为了保持向后兼容性，命令行的默认行为仍是使用“旧”的列表表示法。要激活新的推荐表示法，必须将选项 `MapOptionalAsList` 设置为 `false`。
例如，转换文件时：
`tresos_cmd.bat -DMapOptionalAsList=false legacy convert old.xdm new.xdm`

#### 5.2.5.3. GUI 中的可选元素

在使用 GUI 时，是否使用新的可选特性取决于插件的实现。如果插件的 XDM 文件已经按上述方式转换，GUI 将支持新的表示方式。

---

### 5.2.6. 选择节点 (Choice-Nodes)

在 AUTOSAR 3.0 之前，ECU 配置描述中没有显式的 Choice 结构。但在 XDM DataModel 中，始终存在 Choice Data 节点。

* **Choice Data 节点** (`d:chc`) 不能直接通过简单的 XPath 寻址。
* 相反，路径 `topDef/abc`（在 Data 树中）直接返回**被选中的容器**。
* Choice Data 节点的值是所选容器的 `SHORT-NAME`。

*(图 5.6 展示了 Choice 的表示，以及一个表格对比了不同 AUTOSAR 版本下的 XPath 寻址方式)*

| XPath (Data 树内) | 描述 |
| --- | --- |
| `topDef/abc` | 选择容器节点 "Task2" (即被选中的容器) |
| `ASPath:top/myChoice` (Autosar 2.x) | 选择容器节点 "Task2" |
| `ASPath:top/myAbc/myChoice` (Autosar 3.x) | 选择容器节点 "Task2" |

---

### 5.2.7. 引用 (References)

引用节点的值是目标节点的 `SHORT-NAME` 路径。

*(图 5.7 展示了引用节点的结构)*

| XPath (Data 树内) | 描述 |
| --- | --- |
| `topDef/abc` | 选择引用节点本身 |
| `as:ref(topDef/abc)` | 解析引用，选择目标节点 (例如 'channel2' 容器) |
| `node:ref(topDef/abc)` | 解析引用，选择目标节点 |
| `as:ref('top/channel2')` | 直接通过路径字符串选择目标节点 |

#### 5.2.7.1. URI 引用 (URI References)

AUTOSAR 4.2.1 引入了 URI 引用概念，允许定义特定功能所需的配置参数结构（DestinationUriPolicy），并允许模块链接到该结构。

#### 5.2.7.1.1. XDM 表示

EB tresos Studio 不支持 `DestinationUriPolicy` 的定义，因为这只是一个策略。XDM 只需要目标 URI 的路径。

* **引用定义**：
```xml
<v:ref name="UriRef" type="URI-REFERENCE">
  <a:da name="REF" value="ASPath:/Path/To/DestinationUri"/>
</v:ref>

```


* **容器定义**（作为目标）：
```xml
<v:ctr name="Container" type="IDENTIFIABLE">
  <a:a name="DESTINATION-URI" value="ASPath:/Path/To/DestinationUri"/>
</v:ctr>

```



只要 `REF` 和 `DESTINATION-URI` 的值匹配，引用就可以指向该容器。