### 3. LangGraph 包有哪些核心模块（Modules）？

LangGraph 的设计非常简洁，主要由以下几个核心组件构成：

- **State（状态）**：图的核心血液。通常是一个 Python 字典（TypedDict）或 Pydantic 模型。它在图的各个节点之间传递，每个节点在执行完毕后都会返回一个状态更新（覆盖或追加），下一个节点基于最新的状态继续运行。
    
- **StateGraph（状态图）**：构建图的主类。你将在这里注册所有的节点和边，并最终将其编译（`.compile()`）成一个可执行的应用。
    
- **Node（节点）**：图中执行具体任务的实体。在代码中，节点就是一个普通的 Python 函数或 LangChain 的 Runnable 对象。它接收当前 State 作为输入，输出更新后的 State。
    
- **Edge（普通边）**：定义静态的控制流。例如 `graph.add_edge("node_a", "node_b")`，表示 `node_a` 执行完后，无条件执行 `node_b`。
    
- **Conditional Edge（条件边）**：定义动态的路由逻辑。它是一个包含判断逻辑的 Python 函数。根据当前 State 的具体内容，决定图的下一步应该走向哪个 Node（例如：“如果提取到最终答案，走向 `END` 节点；如果需要更多信息，走向 `Tool` 节点”）。
    
- **Checkpointer（检查点/持久化）**：用于保存图的运行状态。借助它，LangGraph 可以实现“记忆”功能（跨对话保存上下文），或者实现“人类介入（Human-in-the-loop）”，即图运行到某一步暂停，等待人类审批后再继续运行。