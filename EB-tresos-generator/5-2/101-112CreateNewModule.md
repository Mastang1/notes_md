这是文档 **第 101-112 页** 的翻译内容。

这一部分涵盖了 **5.2 节** 的结尾（URI 引用在 XDM 中的表示）以及 **5.3 节：如何创建新模块** 的核心内容，包括创建新模块的先决条件、文件结构设置以及如何配置 `plugin.xml` 中的关键扩展点。

---

### 5.2.7.1.1. XDM 表示 (XDM representation)

EB tresos Studio 不支持定义 `DestinationUriPolicy`。尽管这看起来是该概念的重要组成部分，但对于此特性而言实际上并非必需，因为它仅将参数结构定义为一种策略，实施模块应定义该策略。

唯一需要的是指向目标 URI 的路径。此路径必须对于 URI 引用和定义此参数结构的容器定义（即符合该策略，无论该策略在模型中是否可用）是相同的。

因此，只涉及两个元素：引用和容器——两者都使用相同的目标 URI：

* **引用定义：**
```xml
<v:ref name="UriRef" type="URI-REFERENCE">
  <a:da name="REF" value="ASPath:/Path/To/DestinationUri"/>
</v:ref>

```


* **容器定义（作为目标）：**
```xml
<v:ctr name="Container" type="IDENTIFIABLE">
  <a:a name="DESTINATION-URI" value="ASPath:/Path/To/DestinationUri"/>
</v:ctr>

```



只要 `REF` 和 `DESTINATION-URI` 的值匹配，引用就可以指向该容器。

---

### 5.3. 如何创建新模块 (How to create a new module)

EB tresos AutoCore 可以使用您特定的软件模块进行扩展。本章概述如何在 EB tresos Studio 中创建一个新模块。

**示例：**
输入输出硬件抽象 (IoHwAb) 模块提供了 ECU 内部和外部 I/O 设备的接口。由于每个 ECU 都有其特定的 I/O 能力，因此必须为每个 ECU 实现特定的 IoHwAb 模块。由于其个体特性，IoHwAb 不包含在 EB tresos AutoCore 产品中。

当然，您可以基于适当的 SWCD 和 C 源文件，像集成 SWC 或 CDD 一样集成该模块。但是，如果您希望拥有一个**可配置的** IoHwAb 或将其连接到 PduR，您可以按照本章所述在 EB tresos Studio 中创建 IoHwAb 模块。有关 IoHwAb 的更多信息，请参阅《AUTOSAR I/O 硬件抽象要求》。

> **💡 提示：了解更多关于 IoHwAb 的信息**
> IoHwAb 模块的目的和集成是 EB tresos Classic AUTOSAR 培训的一部分。

#### 5.3.1. 先决条件 (Prerequisites)

您需要一个 **标准化模块定义 (StMD)** 或 **供应商特定模块定义 (VsMD)**。

EB tresos Studio 在 `autosar/<version>` 目录下包含了每个支持的 AUTOSAR 版本的 AUTOSAR StMD 副本（例如 `autosar/4.0.3`, `autosar/4.2.1`, `autosar/R19-11` 等）。

要获取每个模块的单个文件，例如切换到安装目录的 `autosar/R19-11` 目录并执行以下命令：
`create_epds_450.pl AUTOSAR_MOD_ECUConfigurationParameters.arxml`

这将创建多个 `.epd` 文件，其中包含 AUTOSAR 定义模块的 StMD。

#### 5.3.2. 创建供应商特定模块定义 (Creating a vendor-specific module definition)

根据 AUTOSAR 规范，供应商必须从相应的 AUTOSAR StMD 派生出 VsMD。有关更多详细信息，请参阅用户指南中的“创建供应商特定模块定义 (VSMD)”一章。

要将 StMD 文件转换为 VsMD 文件，请使用以下命令：
`tresos_cmd.bat legacy vsmd <StMD> <VsMD> <package> [<ORIGIN>]`

#### 5.3.3. 转换文件 (Converting files)

为了利用 EB tresos Studio 提供的特殊功能，参数定义文件应以 **XDM 格式** 提供。XML 格式允许定义跨参数检查、GUI 注释并添加其他元信息。

要将 AUTOSAR 文件转换为 XDM 格式（或反之），可以使用 `convert` 命令：
`tresos_cmd.bat -DMapOptionalAsList=false legacy convert <infile>... <outfile>`

`convert` 命令也可用于通过提供多个输入文件将不同的文件合并为一个文件。例如：
`tresos_cmd.bat -DMapOptionalAsList=false legacy convert PduR.epd@asc:2.1 PduR.bmd@asc:2.1 PduR.xdm`

#### 5.3.4. 新模块的模板 (Templates for new modules)

EB tresos Studio 在 `demos/Studio` 目录中附带了两个新模块模板：

1. **TemplateGenerator_TS_T00D0M0I0R0**
当应使用 EB tresos Studio 的**基于模板的代码生成器**生成代码时，应使用此模板。
2. **PublicApiTest_TS_T00D0M0I0R0**
如果应在 **Java** 中实现生成器或模块应使用其他 Java 功能，则应使用此模板。

以下章节描述如何设置带有基于模板的代码生成器的模块。

#### 5.3.5. 设置文件结构 (Setup of the file structure)

首先，将目录 `TemplateGenerator_TS_T00D0M0I0R0` 复制到您的开发环境并重命名。
用您按上述方法创建的 VsMD 文件替换文件 `config/TemplateGenerator_TS_T00D0M0I0R0.xdm`。
删除 `generate` 目录中的所有文件和目录。此模板文件只能使用模板插件的未更改的 schematic 文件生成。

#### 5.3.6. 调整 MANIFEST.MF (Adapting MANIFEST.MF)

根据您的需要更改 `META-INF/MANIFEST.MF` 中的以下条目：

* **Bundle-Name**：插件的人类可读名称。
* **Bundle-SymbolicName**：插件的唯一名称——通常建议使用插件的目录名称。
* **Bundle-Vendor**：插件供应商的名称。

#### 5.3.7. 调整 plugin.xml (Adapting plugin.xml)

根据您的需要更改 `plugin.xml`。`plugin.xml` 是用于向 EB tresos Studio 注册资源的 XML 文件。注册按所谓的**扩展 (extensions)** 分组。
对于初步的模块设置，调整演示模块 `TemplateGenerator_TS_T00D0M0I0R0` 中定义的三个扩展就足够了。

##### 5.3.7.1. dreisoft.tresos.launcher2.plugin.module

扩展 `dreisoft.tresos.launcher2.plugin.module` 注册一个新模块。
此扩展中最重要的属性是 `module` 标签下的 `id` 属性。此 ID 将在其他扩展中以 `moduleId` 属性的形式用于引用此模块。

应调整 `extension` 标签的以下属性：

* **id**：插件的唯一 ID（这**不是** moduleId）。
* **name**：插件的人类可读名称。

`module` 标签的实际信息如下：

* **id**：模块的 module-id，此 ID 被 `configuration` 和 `generator` 扩展点引用。
* **label**：模块的人类可读名称。这是将在 GUI 中呈现给用户的模块名称。
* **swVersion**：此模块基于的软件版本。
* **specVersion**：此模块基于的规范版本。
* **relVersion**：此模块基于的发布版本。
* **categoryType**：模块的 Autosar 类型：例如 `Can`, `CanIf`, `Lin`, `LinIf`, `Com`, `Os`...
* **categoryLayer**：模块的层：应为 "Service", "ECU Abs." 或 "MCAL" 之一；用于在配置器中对配置进行分组。
* **categoryCategory**：模块的类别：应为 "IO", "Com", "Diag", "Mem" 或 "Sys" 之一；用于在配置器中对配置进行分组。
* **categoryComponent**：必须是 "ECUC"。

使用 `module` 标签的 **`ecuType`** 子标签，可以限制模块有效的 ECU 目标/派生型号。如果未定义 ECU 类型，则该模块对所有目标有效。

* **target**：模块支持的目标（Target）。
* **derivate**（可选）：模块支持的派生型号（Derivative）。

**示例：**

```xml
<extension point="dreisoft.tresos.launcher2.plugin.module" id="YourID" name="Your plugins name">
  <module id="Your_Module_Id"
          label="Your modules human readable name"
          mandatory="false"
          allowMultiple="false"
          description="Your modules description"
          copyright="YourCopyright"
          swVersionMajor="2" swVersionMinor="0" swVersionPatch="1"
          specVersionMajor="1" specVersionMinor="9" specVersionPatch="0"
          relVersionMajor="2" relVersionMinor="1" relVersionPatch="0"
          categoryType="YourType"
          categoryLayer="Service|ECU Abs.|MCAL"
          categoryCategory="IO|Com|Diag|Mem|Sys"
          categoryComponent="ECUC">
    <ecuType target="YourTarget" derivate="YourDerivate"/>
    <cluster name="Basic Services"/>
    <cluster name="Diagnostic"/>
  </module>
</extension>

```

##### 5.3.7.2. 可选: dreisoft.tresos.launcher2.api.plugin.modulecluster

使用扩展 `dreisoft.tresos.launcher2.plugin.modulecluster`，您可以从外部将模块分配给集群 (Clusters)。

* 使用 `<default>` 标签为未定义集群成员资格的模块提供默认集群分配。
* 使用 `<override>` 标签取代现有的集群分配。
* 使用 `<cluster>` 标签命名一个或多个集群。
* 使用 `<module>` 标签将模块分配给命名集群（支持正则表达式匹配 `regExpModuleId`）。

##### 5.3.7.3. dreisoft.tresos.launcher2.plugin.configuration

扩展 `dreisoft.tresos.launcher2.plugin.configuration` 为模块注册特定于配置的资源，如**参数定义**、**编辑器**、**预配置**和**推荐配置**。

* `configuration` 标签通过属性 `moduleId` 关联到通过 module 扩展定义的模块。
* 该 ID 必须与之前模块注册的 `module` 标签的 id 匹配。

**5.3.7.3.1. 参数定义、预配置、推荐配置**
通过子标签 **`schema`** 可以注册参数定义以及预配置和推荐配置。

* 必须包含一个名为 **`manager`** 的子标签，用于命名加载 schema 资源的 Java 类。必须设置为 `dreisoft.tresos.autosar2.resourcehandling.AutosarSchemaManager`。
* 要加载的文件通过 **`resource`** 标签注册。
* **value**：相对于插件根目录的路径，指向要加载的 XDM 文件。
* **type**：必须设置为 `XDM`。



---

**（第 101-112 页翻译结束）**

接下来的内容（第 114 页起）将涉及：

* **5.3.7.3.2 模块配置 (Module configuration)**：包括包结构。
* **5.3.7.3.3 注册编辑器 (Registering editors)**。
* **5.3.7.4 生成器注册 (dreisoft.tresos.launcher2.plugin.generator)**。
* **5.3.7.5 模块版本 (Module versions)**：软件版本、规范版本等的详细说明。

**是否继续翻译？**