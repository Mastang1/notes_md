
第 5 章：开发模块 (Developing modules) 

#### 5.1. 

概念：XDM 格式 

存储 DataModel（数据模型）的原生持久化格式是 XDM 格式。XDM 格式是一种 XML 数据格式，允许将 DataModel 的节点结构以及所有数据（属性、节点名称）存储在磁盘上。XDM 格式在 DataModel 的第一个版本中也被使用。本章介绍的 XDM 格式是 XDM 7.0 版本。关于可用的 XDM 版本及对应的 EB tresos Studio 版本的概览，请参阅 EB tresos Studio 用户指南中的“XDM 文件的内容类型”一章。

#### 5.1.1. 

概念 

XDM 格式存储以下内容：

* 节点（Schema 节点、Data 节点...）
* 节点属性

为了保持格式的可扩展性，使用了多个 XML 命名空间来区分 DataModel 的元素 。
```xml
<datamodel version="3.0"
           xmlns="http://www.tresos.de/_projects/DataModel2/08/root.xsd"
           xmlns:a="http://www.tresos.de/_projects/DataModel2/08/attribute.xsd"
           xmlns:v="http://www.tresos.de/_projects/DataModel2/06/schema.xsd"
           xmlns:d="http://www.tresos.de/_projects/DataModel2/06/data.xsd">

<d:ctr type="AUTOSAR" factory="autosar"
    xmlns:ad="http://www.tresos.de/_projects/DataModel2/08/admindata.xsd"
    xmlns:icc="http://www.tresos.de/_projects/DataModel2/08/implconfigclass.xsd"
    xmlns:mt="http://www.tresos.de/_projects/DataModel2/11/multitest.xsd" >         
```

<span style="color: red;">DataModel 支持不同种类的节点（为此定义了不同的命名空间）：</span>


* 
**通用节点 (generic nodes)** ：既不是 Schema 节点也不是 Data 节点的节点。


* 
**Schema 节点 (schema-nodes)** ：Schema 节点定义配置参数和配置参数的结构。它们定义 Data 节点的类型、结构和属性默认值。


* 
**Data 节点 (data-nodes)** ：Data 节点代表配置参数。Data 节点被绑定到一个 Schema 节点，该 Schema 节点描述了 Data 节点的有效参数范围。



#### 5.1.2. 

顶层结构 

一个 XDM 文件被包含在 `datamodel` XML 标签中：

```xml
<datamodel version="7.0"
 xmlns="http://www.tresos.de/_projects/DataModel2/16/root.xsd"
 xmlns:a="http://www.tresos.de/_projects/DataModel2/16/attribute.xsd"
 xmlns:v="http://www.tresos.de/_projects/DataModel2/06/schema.xsd"
 xmlns:d="http://www.tresos.de/_projects/DataModel2/06/data.xsd">
</datamodel>

```

**XML 属性说明：** 

* `version`：必须设置为 7.0。
* `xmlns`：声明 DataModel 标签的 XML 命名空间。通用节点标签和 datamodel 标签的命名空间是：`http://www.tresos.de/_projects/DataModel2/16/root.xsd`。
* `factory`：用于创建节点的工厂名称。

**子标签：** 

* 所有节点标签。

`datamodel` 标签本身在数据模型中不表现为一个节点 。

#### 5.1.3. 

节点 (Nodes) 

节点标签用于在数据模型中创建节点。有多种节点标签可用，对应每种节点类型（容器、列表、变量）。XML 命名空间用于将节点分组（Data 节点、Schema 节点...）。所有节点标签都有一个通用的形式：

```xml
<namespace-prefix:core-node-type [name="node-name"] [type="type-string"] [value="value"] [factory="factory-name"] [xmlns:...] >
...
</namespace-prefix:core-node-type>

```

可能的命名空间前缀（例如 'd', 'v', 'a'）在后续章节（属性、通用节点、Schema 节点和 Data 节点）中描述 。
关于 `factory` 和 `xmlns` 属性的详细信息，请参见“5.1.8 工厂”章节 。

**XML 属性说明：** 

* `name`：节点的名称。任何节点都可以有一个名称。某些节点要求其子节点具有唯一的名称。
* `type`：除了节点标签外，可选的 `type` 属性决定了节点类型（为该节点创建的类、允许的子节点、属性...）。
* `value`：每个节点都可以有一个值，通过此属性设置。
* `factory`：用于创建节点的工厂名称。

**子标签：** 

* 所有节点标签
* 所有属性标签

#### 5.1.4. 

属性 (Attributes) 

* **命名空间**：`http://www.tresos.de/_projects/DataModel2/16/attribute.xsd`
* **默认前缀**：`a`

节点可以拥有属性，用于向节点添加元数据。DataModel 属性不应与 XML 属性混淆。这里的属性是通过特殊的 XML 标签添加到节点上的 。
属性有一个名称，并可以有多个字符串值。

属性有三种形式 ：

1. 被动属性 (Passive attributes)
2. 通用属性 (Generic attributes)
3. 自动属性 (Automatic attributes)
##### 5.1.4.1. 

被动属性 (Passive attributes) 

被动属性是指其值在 XDM 文件中定义的属性：

```xml
<a:da name="attribute-name" value="attribute-value"/>

```

或者

```xml
<a:da name="attribute-name">
 <a:v>value</a:v>
</a:da>

```

如果有多个值，或者值包含换行符，则通过 `a:v` 标签设置值 。

##### 5.1.4.2. 

通用属性 (Generic attributes) 

所谓的通用属性能够存储任何（也可以是相当复杂的）数据结构。在 `a:a` 标签内部，每种属性类型都有自己的命名空间，因此每个通用属性看起来都不同。以下解释所有当前定义的通用属性。

5.1.4.2.1. AdminData (管理数据) 
通用属性 "ADMIN-DATA" 专门用于以通用方式保存 Autosar 定义的该属性的复杂数据结构。为此，所有 TAG 和 XML 属性均从 Autosar 格式接管，并以 `ad` 命名空间作为前缀，形式如下：

```xml
<a:a name="ADMIN-DATA" type="ADMIN-DATA">
 <ad:ADMIN-DATA>
  <ad:LANGUAGE>DE</ad:LANGUAGE>
  <ad:USED-LANGUAGES>
   <ad:L-10 L="FR">francais</ad:L-10>
  </ad:USED-LANGUAGES>
  </ad:ADMIN-DATA>
</a:a>

```

除了上面显示的注释外，还可以有 AUTOSAR 定义的更多标签 。

5.1.4.2.2. Implementation Configuration Variant (实现配置变体) 每个 "MODULE-DEF" 可能包含参数 "IMPLEMENTATION_CONFIG_VARIANT"，它通过其 RANGE 定义模块支持的配置变体。参数本身的配置类 (Configuration Class) 对于每个变体都设置为 "PreCompile"，以限制所选配置变体的可变性。示例如下 ：

```xml
<v:var name="IMPLEMENTATION_CONFIG_VARIANT" type="ENUMERATION">
 <a:a name="IMPLEMENTATIONCONFIGCLASS" type="IMPLEMENTATIONCONFIGCLASS">
  <icc:v class="PreCompile">VariantPostBuild</icc:v>
  <icc:v class="PreCompile">VariantPreCompile</icc:v>
 </a:a>
 <a:a name="LABEL" value="Config Variant"/>
 <a:da name="DEFAULT" value="VariantPostBuild"/>
 <a:da name="RANGE">
  <a:v>VariantPostBuild</a:v>
  <a:v>VariantPreCompile</a:v>
 </a:da>
</v:var>

```

如果模块只声明了一个配置变体，则可以省略为 "IMPLEMENTATION_CONFIG_VARIANT" 参数设置配置类 。

5.1.4.2.3. Implementation Config Class (实现配置类) 
属性 `IMPLEMENTATIONCONFIGCLASS` 保存类 (class) 和变体 (variant) 作为值，如下所示：

```xml
<a:a name="IMPLEMENTATIONCONFIGCLASS" type="IMPLEMENTATIONCONFIGCLASS">
 <icc:v class="PreCompile">VariantPreCompile</icc:v>
 <icc:v class="Link">VariantLinkTime</icc:v>
 <icc:v class="PostBuild">VariantPostBuild</icc:v>
 <icc:v class="PostBuildSelectable">VariantPostBuildSelectable</icc:v>
</a:a>

```

5.1.4.2.4. Custom data attribute (自定义数据属性) 
属性 `CUSTOMDATA` 用于保存自定义属性，用户可以将这些属性附加到 Data 节点，例如用一个或多个标签标记一个节点。

```xml
<a:a name="CUSTOMDATA" type="CUSTOMDATA">
 <cd:v id="Label">Some Label</cd:v>
 <cd:v id="Label">Another Label</cd:v>
</a:a>

```

根据附加到 Data 节点的自定义属性的数量，可能会有更多的标签。要定义您自己的自定义数据属性，请参见“6.8. 自定义属性 API”章节 。

##### 5.1.4.3. 

自动属性 (Automatic attributes) 

自动属性通过查询存储在 DataModel 中的配置来计算其自己的值。自动属性有一个 `type` 决定计算值的算法，并且它们配备了描述如何计算值的测试表达式。

自动属性使用以下 XML 标签创建 ：

```xml
<a:da name="attribute-name" type="attribute-type" expr="test-expression" [true="true-value"] [false="false-value"] />

```

或者

```xml
<a:da name="attribute-name" type="attribute-type">
 <a:tst expr="test-expression" [true="true-value"] [false="false-value"] />
</a:da>

```

第二种形式允许定义多个测试表达式 。

属性类型 (Attribute type) 通过当前的 DataModel 工厂映射到实现。`type` 属性决定了自动属性的类型。XML 属性 `test-expression` 的语法由自动属性的类型决定 。

默认支持以下类型 ：

* Bound
* Match
* Range
* XPath
* EcucCond
* EcucValidCond

**注意：** 自动属性不得访问配置类 (configuration class) 晚于自身节点的节点值 。

---

**（第 50-55 页翻译结束）**