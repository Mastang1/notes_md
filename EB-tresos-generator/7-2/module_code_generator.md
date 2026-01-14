# 1. template code tenerator

模板的指定是通过plugin.xml文件中的general配置以及custom配置来实现文件夹路径指定的，指定后扩展点会根据规则转换模板中的代码到输出流，最终写到module指定的输出路径中。


# 2. SELECT / FOR 等迭代
根据 PDF 文档中“Generating code”章节的介绍，`[!SELECT!]` 符号的作用和用法如下：

- **核心功能**：`[!SELECT!]` 用于将当前的节点指针（Context Node Pointer）切换到由 XPath 表达式指定的特定节点上。
- **基本语法**：使用结构为 `[!SELECT "<XPath_Expression>"!] ... [!ENDSELECT!]`。
- **主要特点**：
    - **相对定位**：在 `[!SELECT!]` 块内部，所有后续的 XPath 表达式都会相对于该选中节点进行计算。
    - **与 LOOP 的行为一致**：它在功能执行上与 `[!LOOP!]` 表达式完全相同。
    - **逻辑分工**：虽然技术上与 `[!LOOP!]` 相同，但它通常用于**定位单个节点**（如进入某个特定容器），而不是遍历节点集合。

**应用示例：** ==如果您想访问 `LinGeneral` 容器下的多个参数，可以使用 `[!SELECT "LinGeneral"!]` 切换上下文，后续代码只需直接写参数名（如 `[!"LinVersionInfoApi"! ]`），而无需书写完整的绝对路径。==
