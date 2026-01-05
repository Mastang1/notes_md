这是文档 **第 141-147 页** 的翻译内容。

这一部分详细介绍了 **配置模型 (Configuration models)** 的核心概念（DataModel），以及如何在 XDM 文件中添加 **GUI 注释** 以定制配置编辑器的外观。

---

第 6 章：配置模型 (Configuration models) 

#### 6.1. 

概念：DataModel (The DataModel) 

DataModel 提供了配置数据和其他信息的非常灵活的内存表示。在 EB tresos Studio 中，DataModel 用于表示 AUTOSAR 数据结构，但 DataModel 中包含的数据超出了此范围。

该模型主要分为三个部分：

* **Schematic-tree (Schema 树)**：包含有关所表示数据的元信息。在 AUTOSAR 上下文中，这类似于模块定义 (Module Definition)。
* **Data-tree (Data 树)**：包含配置值。对于每个 Data 节点，在 Schematic-tree 中都存在一个对应的节点。在 AUTOSAR 上下文中，这类似于模块配置 (Module Configuration)。
* **Preferences-tree (偏好设置树)**：存储既不与 Data 树也不与 Schema 树相关的其他一般信息。例如，项目的导入器设置存储在模型的这一部分中。Preferences-tree 不是官方 API。

> **注意：一个模型包含完整的 ECU 配置和定义**
> 在 EB tresos Studio 中，所有 AUTOSAR 模块配置及其模块定义都位于同一个模型中。这意味着您可以访问所有配置数据，甚至定义，而无需考虑您是从哪个模块访问该模型。

所有节点都存储在一个树结构中。因此，每个节点都有一个父节点和任意数量的子节点。只有树的顶层节点没有父节点。

---

*(图 6.1. 简单的节点树)*

配置数据存储在 **Data-tree** 中。此树的结构由 **Schematic-tree** 描述。Schema-tree 和 Data-tree 始终具有相同的结构，这意味着：

* 每个 Data 节点都有一个描述它的 Schema 节点。
* 每个 Data 节点都有一个指向描述它的 Schema 节点的引用。
* Data 节点的父节点指向对应 Schema 节点的父节点。它们之间永远不会有跳跃节点。这确保了 Schematic-tree 和 Data-tree 结构的相似性。

---

*(图 6.2. 简单的 Schematic-tree 和 Data-tree)*

Schematic-tree 和 Data-tree 由不同的节点类型表示：

| Schema 节点 | Data 节点 | 描述 |
| --- | --- | --- |
| **container-node** | **container-node** | 存储固定的节点列表。Schematic-node 包含与 Data-node 相同数量的子节点。 |
| **list-node** | **list-node** | 存储可变的节点列表。Schematic-node 始终只包含**一个**子节点。Data-node 为每个配置（添加）的值包含一个节点。 |
| **choice-node** | **choice-node** | 存储固定的节点列表。Schematic-node 包含所有可用的选项作为子节点。Data-node 包含**所选的**节点作为子节点。 |
| **boolean-node**<br>

<br>**integer-node**<br>

<br>**string-node**<br>

<br>**float-node** | **variable-node** | Schema-node 描述 Data-node 的数据类型和范围。Data-node 存储一个配置条目。这些节点不能包含其他节点。 |
| **reference-node** | **reference-node** | Data-node 引用另一个节点。这些节点不能包含其他节点。 |

#### 6.1.1. 

Schematic-tree (Schema 树) 

Schematic-tree 本身不存储任何配置数据。它只是描述 Data-tree 并提供有关以下的元信息：

* 配置数据变量的类型和范围
* 配置数据变量的默认值
* 可视组件的布局信息
* 工具提示 (tool-tip)
* 在线帮助
* 等

Data 节点的值作为字符串存储，没有进一步的类型信息。因此，只有在引用的 Schema 节点的元信息的支持下，才能正确解释 Data 节点的值。

**6.1.1.1. 示例**
下图中的四个 Data 节点持有相同的字符串值。在 Schema 节点的支持下，这些值获得了它们的数据类型：整数、浮点数、字符串和无效整数（像 RANGE 这样的属性将在 5.1 节 “概念：XDM 格式” 中解释）。

---
![[Pasted image 20260105172155.png]]
*(图 6.3. Schema 和 Data 中的节点)*

**6.1.1.2. Schema 节点的特性**
Schema 节点允许：

* 自动实例化相应的配置 Data 节点
* 填充默认值
* 验证输入的数据
* 渲染允许用户填写配置的对话框。

多个 Data-tree 可以引用同一个 Schema-tree，因为 Schema-tree 仅保存对于所有 Data-tree 都是唯一的元信息。

---

*(图 6.4. 多个 Data-tree 引用一个 Schematic-tree)*

#### 6.1.2. 

Data-Tree (Data 树) 

Data 节点存储配置的值。对于 Schema-tree 中的每个节点，可以在 Data-tree 中创建相应的 Data 节点。

#### 6.1.3. 

Node-Attributes (节点属性) 

每个节点可以有多个属性。每个属性可以存储多个字符串值，并按名称引用。

---

*(图 6.5. 节点和属性)*

例如，`DEFAULT` 属性保存 Data 节点的默认值。`RANGE` 属性可能包含定义 Data 节点范围的多个值。

当从 XDM 文件加载 Schema 或配置数据时，属性是按 XDM 文件中的定义显式设置的。当加载 AUTOSAR 配置格式时，属性是隐式设置的（例如，`RANGE` 属性将从 `ENUM-LITERAL-DEF` 计算得出）。

DataModel 识别两种类型的属性：

* **被动属性 (Passive attributes)**：值被显式设置。
* **自动属性 (Automatic attributes)**：它们的值将在每次树中的数据更改时计算，因为它们的值取决于当前配置。它们的值不能显式设置。

#### 6.1.4. 

DataModel 中的列表表示 (List-Representation within the DataModel) 

列表节点 (list-node) 的一个特点是 Data 中的子节点可能比 Schema 中的多。在 Schema-tree 中，一个 list-node 始终**恰好有一个**子节点。这个子节点描述了 Data-tree 中相应 list-node 的子节点。Data-tree 中的所有列表子节点都引用 Schema-tree 中的那个单一列表子节点。

---

*(图 6.6. 列表表示)*

#### 6.1.5. 

DataModel 中的选择表示 (Choice-representation within the DataModel) 

选择 (Choice) 提供了从子节点列表中选择单个节点的可能性。

* 在 **Schema-tree** 中，所有可用的节点都是 choice-node 的子节点。
* 在 **Data-tree** 中，choice 包含一个代表**用户所选节点**的子节点。Data 中的 choice-node 提供所选节点的名称作为节点值。

---

*(图 6.7. 选择表示)*

---

### 6.2. 

如何在 XDM 文件中添加 GUI 注释 (How to add GUI annotation in the XDM file) 

#### 6.2.1. 

目的 (Purpose) 

DataModel 以及 XDM 格式支持在参数定义中添加 GUI 注释，以便更精细地控制配置在通用配置编辑器中的呈现方式。本章介绍了所有这些结构，并描述了如何在 XDM 中制定它们。

#### 6.2.2. 

通用配置编辑器布局 (Generic Configuration Editor Layout) 

本章详细介绍了配置模型如何在 GUI 中布局。它假设 AUTOSAR 参数定义已经构成了配置的基础。通用配置编辑器读取参数定义并创建相应的 GUI。

通用配置编辑器由两部分组成：

1. **编辑器本身**
2. **节点大纲 (Node Outline)**

编辑器显示页面，其中一次只有一个页面可见。通用配置器为 `MODULE-DEF` 元素创建一个页面，并为属于列表一部分的每个容器（`UPPER-MULTIPLICITY` 或 `LOWER_MULTIPLICITY != 1`）创建一个页面。

页面有一个标题和一个允许编辑名称（底层容器的 `SHORT-NAME`）的条目。如果页面代表模块定义，则名称不可编辑。

---

*(图 6.8. 页面顶部)*

页面包含用于该页面所代表容器下的所有配置参数、子容器和选择的编辑元素。

页面内容被分组到**选项卡 (Tabs)** 中。

* 如果存在 `UPPER-MULTIPLICITY` 或 `LOWER_MULTIPLICITY != 1` 的参数（在这种情况下，参数、容器或选择在 DataModel 中由列表表示），则会创建一个 **General**（常规）选项卡。
* 所有列表元素（`UPPER-MULTIPLICITY` 或 `LOWER_MULTIPLICITY != 1`）以及实例引用都存储在单独的页面上，该页面以元素的名称标记。