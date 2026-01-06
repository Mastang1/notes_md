- ==**注意：plugin-config文件夹中的xdm文件作为模板使用，节点类型属于schema; 在生成的config_name.xdm文件中，节点类型属于schema；==**
# 1. list-container generate a table filled some elements
在 `Lin_43_LLCE.xdm` 片段中（参考 507-522 行及后续内容），`lst`、`container` 与 `variable` 构成了“集合、模板与成员”的层级关系：

### 1. 节点间的逻辑关系

- **`lst` (列表节点)**：作为包装器，定义了多重性（如 `LinChannel` 列表）。在 Schema 树中，它负责规定该集合可以包含多少个实例（MIN/MAX）。
- **`container` (容器子节点)**：它是列表中的==“行模板”==。在 Schema 中，`lst` 下通常只定义一个 `v:ctr`，它规定了列表中每一个条目应该具备的结构。
- **`variable` (变量子节点)**：是容器内的具体配置参数。在 `LinChannel` 容器下，`LinChannelId`、`LinNodeType` 等变量是该通道配置的核心数据单元。

### 2. 信息如何“插入”到 List 中

信息通过以下两种机制从子节点整合到列表中：

- **数据实例化（Data Instantiation）**：当用户在工具中向列表添加新项时，系统会基于 Schema 中的 `v:ctr` 模板在数据树（Data Tree）中创建一个新的容器实例。该实例会自动包含所有预定义的 `v:var` 子节点。
- **UI 表格映射（Table Mapping）**：如果 `v:lst` 的类型设置为 `MAP`（如 `Lin_43_LLCE.xdm` 中常见的做法），可以通过 `<a:a name="COLUMNS">` 属性将容器内部变量（如 `LinChannelId`）的路径映射为表格的列。这样，容器内部的变量信息就会以“行数据”的形式直观地插入并显示在列表的 UI 视图中。

**示例（基于 Lin_43_LLCE.xdm 结构）：**

```
<v:lst name="LinChannel"> <!-- 集合 -->
  <v:ctr name="LinChannel" type="IDENTIFIABLE"> <!-- 每一个通道的模板 -->
    <v:var name="LinChannelId" type="INTEGER"/> <!-- 具体的成员数据 -->
    <v:var name="LinNodeType" type="ENUMERATION"/> <!-- 具体的成员数据 -->
  </v:ctr>
</v:lst>
```

您是否需要我为您演示如何在代码模板中使用 `[!LOOP!]` 指令来遍历这些插入到列表中的通道参数？

# 2. list-container demo
本演示结合 `Lin_43_LLCE.xdm` 模块配置、XPath 寻址逻辑以及代码模板宏，展示从界面定义到自动化代码生成的完整流程。

### 1. XDM 配置演示：定义带动态校验的 GUI 元素

在 XDM 文件中，通过容器（`v:ctr`）和变量（`v:var`）定义参数树。利用 `a:a` 设置静态界面属性，利用 `a:da` 实现基于 XPath 的动态交互逻辑。

```
<!-- 演示：定义 LIN 通道列表及动态校验 -->
<v:lst name="LinChannel" type="MAP">
  <!-- 手动将此列表分配到特定的 Tab 选项卡 -->
  <a:a name="TAB" value="Channel Config"/>

  <v:ctr name="LinChannel" type="IDENTIFIABLE">
    <!-- 通道 ID 变量：包含静态标签和动态唯一性校验 -->
    <v:var name="LinChannelId" type="INTEGER">
      <a:a name="LABEL" value="LIN Channel ID"/> <!-- UI 显示名称 -->
      <a:da name="DEFAULT" value="0"/> <!-- 默认值 -->

      <!-- 动态属性 a:da：使用 node:uniq 函数确保 ID 在所有通道中唯一 -->
      <a:da name="INVALID" type="XPath"
            expr="node:uniq(../../*/LinChannelId, .)"
            false="Duplicated value, LinChannelId must be unique across all channels."/>
    </v:var>

    <!-- 节点类型变量：通过可见性属性实现 UI 联动 -->
    <v:var name="LinNodeType" type="ENUMERATION">
      <a:da name="RANGE">
        <a:v>MASTER</a:v>
        <a:v>SLAVE</a:v>
      </a:da>
    </v:var>
  </v:ctr>
</v:lst>
```

### ==补充说明==

- ==a:v中的属性信息，主要用于错误校验、范围校验等，具体的data value在UI运行generate configuration时候生成，存储到同名的xdm文件中；
- ==generate code：应用的是生成的configuration文件，调用基于模板的EB生成工具生成配置代码==
在 `Lin_43_LLCE.xdm` 文件中，针对 `LinChannelId` 节点的配置包含了多种属性和子插件，它们在 EB tresos 中分别承担着定义数据类型、配置元数据、预设值以及动态校验的任务：

### 1. 节点基础属性 (XML Attributes)

- **`name="LinChannelId"`**：定义节点的名称，在 DataModel 中具有唯一性标识作用。
- **`type="INTEGER"`**：指定该变量的数据类型为 64 位有符号整数，界面将呈现为数字输入框。

### 2. 静态/被动属性 (`a:a` 标签)

- **`DESC`**：提供该参数的详细描述，内容会显示在“Description”视图或悬停提示（Tooltip）中，支持 HTML 格式和链接。
- **`IMPLEMENTATIONCONFIGCLASS`**：这是一种通用属性（Generic Attribute），用于定义该参数在不同变体（如 `VariantPreCompile` 或 `VariantPostBuild`）下的配置类。

### 3. 动态/自动属性 (`a:da` 标签)

这类属性不存储固定值，而是通过表达式动态计算其在 UI 上的表现：

- **`DEFAULT`**：定义该节点的初始默认值。当用户点击界面上的“Calculate”按钮时，系统会根据此属性重新填充值。
- **`INVALID (type="XPath")`**：
    - **作用**：执行实时逻辑校验。
    - **逻辑**：通过 `node:uniq` 函数检查当前路径下所有通道的 `LinChannelId` 是否唯一。
    - **UI 影响**：如果检测到重复值，UI 会显示 `false` 属性中定义的错误消息：“Duplicated value...”，并阻止生成过程。
- **`INVALID (type="Range")`**：
    - **作用**：执行数值边界校验。
    - **逻辑**：使用 `Range` 类型确保输入值小于等于 4（硬件限制）。
    - **UI 影响**：若输入值超出范围，界面会显示对应的报错信息，指导用户修正输入。

这段配置完整展示了 XDM 如何通过 `a:da` 将简单的参数输入框转变为一个具备自动化计算和实时错误拦截功能的智能控件。您是否需要了解如何编写这些 `INVALID` 属性中的具体 XPath 函数？


### 2. XPath 演示：节点定位与数据检索

XPath 是在 DataModel 树状结构中导航的核心。它可以检索具体配置值或用于校验表达式。

- **绝对路径定位**：`/AUTOSAR/TOP-LEVEL-PACKAGES/Lin/ELEMENTS/Lin/LinGlobalConfig` 可定位至 Lin 模块的全局配置根节点。
- **谓词过滤（Predicates）**：`./LinChannel/*[@name='LinChannel_0']` 用于在列表中精确选择名称为 "LinChannel_0" 的容器。
- **引用解析**：`node:value(node:ref(LinClockRef))`。该表达式先通过 `node:ref` 定位引用的目标节点（如 Mcu 时钟），再通过 `node:value` 获取该节点的实际频率值。

### 3. 代码生成宏演示：根据配置动态输出 C 代码

在 `.c` 或 `.h` 模板文件中，使用 `[!宏!]` 命令提取 DataModel 中的数据并根据逻辑生成代码。

```
[!CODE!] [!// 开启生成流输出
/* LIN 驱动通道配置 - 自动生成 */
#include "Lin_43_LLCE_Cfg.h"

[!VAR "MaxChannels" = "count(LinGlobalConfig/LinChannel/*)"!] [!// 定义变量存储通道总数

const Lin_43_LLCE_ChannelConfigType Lin_Config_Data[[!"num:i($MaxChannels)"!]] = {
[!LOOP "LinGlobalConfig/LinChannel/*"!] [!// 遍历 XDM 中配置的所有通道
    {
        /* 通道名称: [!"as:name(.)"!] */ [!// 获取 AUTOSAR SHORT-NAME
        .u8LinChannelID = [!"LinChannelId"!]U,
        .eLinNodeType = [!IF "contains(node:value(LinNodeType), 'MASTER')"!]LLCE_LIN_MASTER_NODE[!ELSE!]LLCE_LIN_SLAVE_NODE[!ENDIF!], [!// 逻辑分支

        [!SELECT "node:ref(LinClockRef)"!] [!// 切换上下文至引用的 MCU 时钟节点
            /* 计算后的波特率寄存器值，基于时钟频率 [!"McuClockReferencePointFrequency"!] Hz */
            [!INCLUDE "Lin_43_LLCE_BaudRate_Comp.m"!] [!// 包含外部波特率计算宏
        [!ENDSELECT!]
        .u32LinBaudRateRegValue = [!"$LINIBRR_LINFBRR"!]U
    }[!IF "not(node:islast())"!],[!ENDIF!] [!// 仅在非最后一个元素后添加逗号
[!ENDLOOP!]
};
[!ENDCODE!]
```

### 演示逻辑总结：

1. **数据流向**：用户在 GUI 中修改 `LinChannelBaudRate` -> 触发 XDM 中的 `a:da` 校验表达式 -> 校验通过后，生成器通过 `[!LOOP!]` 访问该值 -> 通过波特率计算宏（`.m` 文件）处理数据 -> 输出至最终的 `Lin_43_LLCE_PBcfg.c` 文件。
2. **UI 自动化**：由于在 XDM 中使用了 `v:lst` 且配置了 `a:a name="TAB"`，EB tresos 会自动为通道配置创建一个独立的 Tab 页，并以表格形式展示多行配置数据。
3. **代码一致性**：利用 `[!ASSERT!]` 或 `[!ERROR!]` 宏，如果配置不符合硬件约束（如频率超出范围），生成过程会立即停止并向控制台报错，防止生成无效的 C 代码。

您是否需要我为您演示如何结合 **Post-Build 变体（Variant）** 功能，为不同的硬件平台生成多套差异化的配置代码？