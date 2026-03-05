这是一份为你整理的硬核执行笔记。去掉了所有冗余的理论铺垫，直接将 AI 应用开发映射到你熟悉的系统级开发概念上。

## 📓 学习笔记：从底层逻辑构建 AI Agent 应用

**核心架构思维**：

- **大模型 (LLM)** = 输入输出完全解耦的“纯字符串处理函数库”。
    
- **Agent** = 带有上下文记忆和环境状态的 `While(True)` 状态机。
    
- **Tool Calling** = 软硬件交互中的“中断回调 (Callback)”。
    

### 🧱 阶段 0：紧急环境整备（优先执行）

在网络环境受限前，完成本地“弹药库”的物理隔离部署。

- **动作 1：** 终端执行 `ollama pull qwen2.5-coder:7b`（储备本地断网可用的代码级大模型）。
    
- **动作 2：** 环境执行 `pip install sentence-transformers faiss-cpu` 并运行极简脚本缓存 `BAAI/bge-m3` 向量模型。
    
- **动作 3：** 执行 `git clone https://github.com/modelcontextprotocol/python-sdk.git` 将 MCP 官方协议库克隆至本地 Ubuntu 目录。
    

---

### 🟢 阶段一：裸调 API 与 JSON 格式化 (基石)

**目标**：把 AI 变成一个可控的、能输出标准数据结构的系统组件。

**对标概念**：底层通信协议的数据包格式化与解析。

- **执行步骤**：
    
    1. **舍弃框架**：仅使用 Python 内置的 `requests` 和 `json` 库。
        
    2. **配置总线**：通过 `System Prompt` 强行约束模型行为（例如：“你是一个严格的日志解析器，仅输出 JSON”）。
        
    3. **闭环验证**：向 API 传入一段乱码日志，要求提取特定字段。如果在 Python 层用 `json.loads()` 解析失败，说明 Prompt 不及格，继续调优直到 100% 成功。
        
- **推荐学习资源**：
    
    - **DeepSeek API 官方文档 (Chat Completions 与 JSON Mode 章节)**：不用看别的，它的接口规范完全兼容 OpenAI 标准，直接看如何构造 Request Payload 和配置参数即可。
        

---

### 🟡 阶段二：Function Calling / 函数调用实战

**目标**：让 AI 知道本地有哪些 Python 函数可用，并在需要时触发它们。

**对标概念**：配置外设寄存器，并响应底层硬件中断。

- **执行步骤**：
    
    1. **定义接口**：写一个本地探针函数，如 `get_ubuntu_cpu_temp()`。
        
    2. **编写描述符 (JSON Schema)**：把函数的入参类型、返回值和功能描述写成严格的 JSON 结构，随提问一起发给 LLM。
        
    3. **拦截指令**：解析 LLM 返回的 JSON，提取出它想调用的函数名。
        
    4. **执行与回传**：在本地 Python 环境主动执行该函数，并将拿到的真实数据（如“45度”）二次发给 LLM，完成完整推理。
        
- **推荐学习资源**：
    
    - **OpenAI Cookbook (GitHub 仓库中的 Function Calling 示例代码)**：业界标准的实操指南。找一个最简单的 Python Jupyter Notebook 示例，逐行阅读它的 JSON Schema 是如何编写的。
        

---

### 🟠 阶段三：手搓极简 RAG (检索增强生成)

**目标**：让 AI 能够读取并理解本地 C 语言工程代码、MCAL 手册等专业文档。

**对标概念**：为单片机外接一块 Flash 存储器，并建立内存映射表。

- **执行步骤**：
    
    1. **切片 (Chunking)**：用 Python 将大文档（如 PDF 或 `.h` 头文件）按 500 字符切分成数据块。
        
    2. **向量化 (Embedding)**：调用本地的 `sentence-transformers`，把文字变成多维浮点数组。
        
    3. **建表与检索**：把向量塞入 FAISS (本地内存数据库)。当有新问题输入时，同样向量化并计算余弦相似度，找出最匹配的 3 个数据块。
        
    4. **上下文拼接**：把找出的内容作为背景信息，拼接到给 LLM 的 Prompt 中。
        
- **推荐学习资源**：
    
    - **FAISS GitHub 官方 Wiki & `sentence-transformers` 官方快速入门**：避开 LangChain 的复杂封装，直接看这两个底层库的文档，几十行代码就能写透 RAG 的本质。
        

---

### 🔴 阶段四：MCP 协议攻坚与系统集成

**目标**：打通应用层与系统底层的连接，让 AI 真正拥有控制操作系统的能力。

**解决核心痛点**：接管终端，处理 OpenClaw 无法直接操作 Ubuntu 界面的问题。

- **执行步骤**：
    
    1. **理解协议栈**：解析 MCP (基于 stdio 或 SSE 的 JSON-RPC 协议)。
        
    2. **编写 Server**：利用克隆好的 MCP Python SDK，手搓一个本地 Server 端。在内部暴露一个名为 `execute_bash` 的 Tool，使用 Python 的 `subprocess.run()` 真实执行 Shell 命令。
        
    3. **挂载测试**：在 OpenClaw 的配置文件中，将你写好的 Python Server 路径配置进去。
        
    4. **联合调试**：在前端聊天界面输入系统级命令，验证 OpenClaw 是否能通过 MCP 协议唤起你的底层 Python 脚本并拿到回显。
        
- **推荐学习资源**：
    
    - **Anthropic 官方的 Model Context Protocol (MCP) 文档与开源示例库 (GitHub: modelcontextprotocol/servers)**：重点研究官方提供的 `filesystem` 或 `sqlite` server 的 Python 源码，模仿它们的数据流转逻辑。

#
