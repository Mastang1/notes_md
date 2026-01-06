XPath (XML Path Language) 是一门在 XML 文档中查找信息的语言。虽然它最初是为了 XML 设计的，但在今天的 **Web 数据抓取 (Web Scraping)**、**UI 自动化测试 (如 Selenium)** 以及 **HTML 解析**中，它都是一项极其核心且强大的技术。

可以将 XPath 想象成 **XML/HTML 文档的“文件路径系统”**。就像你在电脑中用 `C:/Users/Documents` 找到文件一样，XPath 用类似的路径语法在复杂的标签树中定位具体的元素。

以下是 XPath 的核心概念详解、语法说明及实例。

---

### 0. 预备知识：示例 XML 文档

为了让你更好地理解后续的概念，我们使用以下这段简单的 XML 代码作为演示对象：

XML

```
<bookstore>
  <book category="cooking">
    <title lang="en">Everyday Italian</title>
    <author>Giada De Laurentiis</author>
    <year>2005</year>
    <price>30.00</price>
  </book>
  <book category="children">
    <title lang="en">Harry Potter</title>
    <author>J K. Rowling</author>
    <year>2005</year>
    <price>29.99</price>
  </book>
</bookstore>
```

---

### 1. 节点 (Nodes) 与 节点树 (Tree)

XPath 将 XML 文档视为一棵**树 (Tree)**。文档中的每一个部分（元素、属性、文本等）都被称为**节点 (Node)**。

- **根节点 (Root Node):** 整个文档的顶层（在上面的例子中是 `<bookstore>` 的父级，虽然不可见，但在逻辑上存在）。
    
- **元素节点 (Element Node):** 如 `<book>`, `<title>`, `<author>`。
    
- **属性节点 (Attribute Node):** 依附于元素的属性，如 `category="cooking"` 中的 `category`。
    
- **文本节点 (Text Node):** 标签内的实际文字，如 "Everyday Italian"。
    

> **核心概念：** 节点之间存在层级关系，包括 **父 (Parent)**、**子 (Child)**、**兄弟 (Sibling)**、**先辈 (Ancestor)** 和 **后代 (Descendant)**。

---

### 2. 基本路径表达式 (Path Expressions)

这是 XPath 最基础也是最常用的部分。

|**符号**|**描述**|**示例**|**结果**|
|---|---|---|---|
|**`nodename`**|选取此节点的所有子节点|`bookstore`|选取 bookstore 元素|
|**`/`**|**从根节点选取** (绝对路径)|`/bookstore`|选取根元素 bookstore<br><br>  <br><br>_(如果写 `/book` 则选不到，因为 book 不是根)_|
|**`//`**|**从匹配选择的当前节点选择文档中的节点，而不考虑它们的位置** (相对路径)|`//book`|选取文档中**所有**的 book 元素，无论它们在哪一层|
|**`.`**|选取当前节点|||
|**`..`**|选取当前节点的父节点|||
|**`@`**|**选取属性**|`@lang`|选取名为 lang 的属性|

**常用组合示例：**

- `//title[@lang]`: 选取所有拥有 `lang` 属性的 title 元素。
    
- `//book/title`: 选取所有 book 元素下的直接子元素 title。
    

---

### 3. 谓语 (Predicates) —— 精确查找

谓语用来查找某个特定的节点或者包含某个指定值的节点。谓语被嵌在方括号 **`[ ... ]`** 中。这是 XPath 强大的过滤功能的体现。

#### A. 按索引位置查找

_注意：XPath 的索引通常**从 1 开始**，而不是 0。_

- `/bookstore/book[1]`: 选取 bookstore 下的第一个 book 元素。
    
- `/bookstore/book[last()]`: 选取 bookstore 下的最后一个 book 元素。
    
- `/bookstore/book[last()-1]`: 选取倒数第二个 book 元素。
    

#### B. 按属性或子元素值查找

- `//title[@lang='en']`: 选取所有 `lang` 属性值为 `en` 的 title 元素。
    
- `//book[price>35.00]`: 选取所有 `price` 元素值大于 35.00 的 book 元素。
    
- `//book[price]`: 选取所有拥有 `price` 子元素的 book 元素（不关心价格是多少，只要有这个标签就行）。
    

---

### 4. 通配符 (Wildcards)

当你不知道具体的元素名称，或者想要选取所有类型的元素时使用。

| **通配符**  | **描述**   | **示例**         | **说明**                 |
| -------- | -------- | -------------- | ---------------------- |
| **`*`**  | 匹配任何元素节点 | `/bookstore/*` | 选取 bookstore 下的所有子元素   |
| **`@*`** | 匹配任何属性节点 | `//title[@*]`  | 选取所有带有至少一个属性的 title 元素 |

---

### 5. 轴 (Axes) —— 高级导航

当简单的父子关系 (`/`) 无法满足需求时（例如：你需要找“某个元素的叔叔”或者“某个元素之后的所有兄弟”），就需要使用**轴**。282930

语法格式：`轴名称::节点测试[谓语]`

**最常用的轴：313233**

1. **`parent`**: 选取当前节点的父节点。3435
    
    - 例：`//title/parent::*` 36(这实际上会选中 book 元素)37。
        
2. **`child`**: 选取当前节点的所有子元素（默认轴，通常省略）。
    
3. **`ancestor`**: 选取当前节点的所有先辈（父、祖父等）。
    
4. **`descendant`**: 选取当前节点的所有后代（子、孙等）。
    
    - 例：`/bookstore/descendant::title` (等同于 `/bookstore//title`)。
        
5. **`following-sibling`**: 选取当前节点**之后**的所有同级节点。
    
    - _场景：在表格中，定位到“姓名”这一列表头，然后想找它后面的“年龄”表头。_
        
6. **`preceding-sibling`**: 选取当前节点**之前**的所有同级节点。
    

---

### 6. 常用函数 (Functions) & 运算符

在爬虫和自动化测试中，仅仅靠路径往往不够，我们需要基于文本内容进行逻辑判断。

#### A. 模糊匹配 (非常重要)

在 HTML class 经常变化的现代网页中，模糊匹配是神器。

- **`contains()`**: 判断属性或文本是否包含某字符串。
    
    - `//title[contains(text(), 'Harry')]`: 选取文本内容包含 "Harry" 的 title。
        
    - `//div[contains(@class, 'nav-bar')]`: 选取 class 属性中包含 "nav-bar" 的 div（能匹配 `class="main nav-bar top"`）。
        
- **`starts-with()`**: 判断是否以某字符串开头。
    
    - `//input[starts-with(@id, 'user_')]`: 选取 id 以 "user_" 开头的 input 标签。
        

#### B. 文本获取

- **`text()`**: 选取节点的文本内容。
    
    - `/bookstore/book/title/text()`: 结果是 "Everyday Italian" (文本节点)，而不是 `<title>Everyday Italian</title>` (元素节点)。
        

#### C. 逻辑运算

- **`|` (管道符)**: 计算两个节点集的并集。
    
    - `//title | //price`: 选取文档中所有的 title 和所有的 price。
        
- **`and` / `or`**: 逻辑与/或。
    
    - `//book[@category='cooking' and price<35]`.
        

---

### 7. 总结：如何写出健壮的 XPath？

1. **优先使用相对路径 (`//`)**: 除非你非常确定结构永远不变，否则尽量避免从根节点 `/html/body/div...` 开始写，因为网页结构一变，路径就失效了。
    
2. **利用属性定位**: ID 是最好的定位符 (`//*[@id='unique']`)，其次是独特的 Class。
    
3. **善用 `contains`**: 处理动态生成的 ID 或复杂的 Class 组合时，`contains` 比全等匹配更稳定。
    
4. **文本定位**: 如果没有好的属性，可以尝试通过显示的文字定位，如 `//button[text()='提交']`。
    

---

我可以为你做一个针对性的练习：

如果你把你正在处理的一段 HTML 代码发给我，我可以为你写出提取特定数据的最佳 XPath 表达式。