## 0. 协议
这是一个非常硬核且关键的问题。要真正掌握大模型的底层交互，把 API 的 Request 和 Response 拆解得清清楚楚是必经之路。

我们先来拆解第一部分：LLM 交互的核心 JSON 结构（以行业内事实上的标准格式为例），然后再对比 OpenAI 和 Ollama 在协议上的差异。

---

### 一、 LLM 输入输出 JSON 核心 Key 详解

在涉及“工具调用 (Tool Calling)”的对话中，JSON 报文的复杂度会显著增加。以下是核心字段的逐一拆解：

#### 1. Request (输入 JSON)

这是你（或者你的程序）发送给大模型服务器的数据包。

| 核心 Key | 数据类型 | 作用与说明 |
| --- | --- | --- |
| **`model`** | String | 指定要调用的模型名称，例如 `"gpt-4o"` 或 `"llama3.1"`。 |
| **`messages`** | Array | **最核心的字段。** 按时间顺序记录所有对话历史的数组。LLM 是无状态的，必须每次都把历史记录全传过去。 |
| **`tools`** | Array | **工具说明书。** 告诉模型当前有哪些本地函数可用。通常包含 `type` (一般固定为 `"function"`) 和 `function` (包含函数 `name`、`description` 和用 JSON Schema 描述的参数格式 `parameters`)。 |
| **`tool_choice`** | String/Object | 控制模型是否必须使用工具。 `"auto"` (模型自行决定，默认值)、 `"none"` (强制不使用)、或指定某个具体的函数名强制调用。 |
| **`temperature`** | Float | 控制输出的随机性 (0.0 到 2.0)。数值越低越严谨，做 Agent 开发或代码生成时通常设为 `0` 或 `0.1` 以保证输出稳定。 |
| **`max_tokens`** | Integer | 限制模型输出的最大 Token 数量，防止模型无休止地生成内容或超出预算。 |

**`messages` 数组内部对象详解：**
每一条 message 必须包含 `role` 和 `content`。在 Tool Calling 场景下，还会增加特定的 Key：

* **当 role 为 `user` 时：** `content` 是用户的提问。
* **当 role 为 `assistant` 时：** 如果模型决定调用工具，`content` 可能为空，但会多出一个 **`tool_calls`** 数组，里面包含模型生成的函数名和参数。
* **当 role 为 `tool` 时：** 必须包含 **`tool_call_id`** (对应 assistant 发起的调用 ID) 和 `content` (本地函数的执行结果字符串)。

#### 2. Response (输出 JSON)

这是大模型处理完毕后，返回给你的数据包。

| 核心 Key | 数据类型 | 作用与说明 |
| --- | --- | --- |
| **`id`** | String | 本次请求的全局唯一标识符，常用于日志追踪。 |
| **`model`** | String | 实际生成回复的模型名称。 |
| **`choices`** | Array | 模型的生成结果。通常只包含一个元素（除非你设置了生成多个候选结果）。 |
| **`usage`** | Object | 计费与性能指标。包含 `prompt_tokens` (输入消耗)、`completion_tokens` (输出消耗) 和 `total_tokens`。 |

**`choices[0]` 内部对象详解：**

* **`message`**: 包含模型回复的实体，结构与输入中的 message 一致（包含 `role: "assistant"`、`content`，如果有工具调用则包含 `tool_calls`）。
* **`finish_reason`**: **极其重要的状态码**，决定了你下一步程序的走向。
* `"stop"`: 模型正常生成完毕，输出了自然语言。
* `"tool_calls"`: 模型暂停了生成，请求你执行本地工具。
* `"length"`: 达到了最大 Token 限制，生成被截断。



---

### 二、 OpenAI 协议 vs Ollama 协议对比

你之所以会觉得有些混乱，是因为现在处于一个“群雄割据走向统一”的阶段。

**OpenAI 协议是目前的“行业普通话”。** 无论是国内的 DeepSeek、通义千问，还是国外的 Claude (通过某些中间件)、Mistral，都在努力让自己的 API 格式向 OpenAI 看齐，因为这样可以无缝接入现有的庞大生态（如 LangChain、Roo Code 等）。

**Ollama 作为一个本地大模型运行器，经历了从“自有协议”到“兼容 OpenAI 协议”的转变。**

以下是两者的核心差异对比：

| 维度 | OpenAI 协议 | Ollama 协议 (原生 API) | 区别与注意事项 |
| --- | --- | --- | --- |
| **设计初衷** | 面向云端商业化调用，结构严谨，字段复杂，高度标准化。 | 面向本地快速拉起和测试，设计极为精简，降低个人开发者门槛。 | OpenAI 协议是目前的行业标准；Ollama 原生协议更轻量。 |
| **默认 Endpoint** | `https://api.openai.com/v1/chat/completions` | `http://localhost:11434/api/chat` | 路径设计不同。 |
| **鉴权 (Auth)** | 强依赖 HTTP Header 中的 `Authorization: Bearer <API_KEY>` | 默认无鉴权，直接调用本地服务。 | 在企业级开发中，调用本地 Ollama 时通常需要自己在网关层加鉴权。 |
| **模型能力依赖** | 只要是支持 Tool Calling 的模型 (如 gpt-4o)，必然严格遵守 JSON Schema 输出参数。 | 取决于你拉取的**本地模型本身的素质**。 | 这是最容易踩坑的地方！你用 Ollama 跑 `llama3.1`，它可以完美按协议输出工具调用；如果你跑一个较弱的模型，它可能根本无视你的 `tools` 字段，直接用普通文本回答。 |
| **流式输出控制** | 默认 `stream: false`，需显式开启并处理 SSE (Server-Sent Events) 格式。 | 默认 `stream: true` (在某些 SDK 中)，按行返回 JSON 块。 | 处理逐字打印时的解析逻辑有所不同。 |

#### 💡 关键转折：Ollama 的 OpenAI 兼容模式

为了解决生态孤岛的问题，Ollama 后来做了一个非常聪明的决定：**内置了 OpenAI 兼容层。**

这意味着，现在你**完全可以把 Ollama 当作 OpenAI 来用**。
你只需要把原本调用 OpenAI 的代码稍微改两处：

1. 把请求地址从 `https://api.openai.com/v1/...` 改为 `http://localhost:11434/v1/...`
2. 把模型名称换成本地已有的模型（如 `"llama3.1"`）。

这也是为什么现在像 Roo Code 或 Cline 这样的工具，可以直接接入本地 Ollama 的原因——它们内部发出的都是标准的 OpenAI 格式 JSON，而 Ollama 的 `/v1/chat/completions` 接口能够完美接收并解析这些报文，然后驱动本地模型工作。

---
## 1. agent skill 定义
没有问题，将知识体系化并沉淀为笔记，是掌握复杂工程概念的最佳方式。

为了让你对 Skill 的构成有更直观的体感，我为你设计了一个在嵌入式自动化测试或驱动开发中非常实用的最小 Skill 示例——**计算数据帧的校验和 (Checksum)**。

下面是为你整理的完整学习笔记，包含了核心概念的梳理以及你所需要的最小 Skill 完整切片。

---

# 学习笔记：LLM、Skill 与 Agent 的底层架构与工程实现

## 一、 核心概念：这三者到底是什么关系？

在 AI 应用开发中，不要被各种新名词迷惑，一切都可以映射为传统的软件工程概念：

* **API (通信协议)**：最底层的 JSON 数据交换协议，就像网络通信中的 TCP/IP 报文，或者是 SoC 内部 IPC 通信的 Payload 标准。
* **Skill/Tool (技能/工具)**：Agent 的“手和脚”。在代码层面，它就是一个普通的本地函数；在与 LLM 交互时，它体现为一份 JSON 格式的“接口说明书”。
* **Agent (智能体)**：一个包含“循环控制流 (While Loop)”的自动化系统。它负责调度大模型（大脑）和各种 Skill（手脚），通过不断地“思考 -> 调用工具 -> 获取结果 -> 再思考”，直至完成复杂任务。

## 二、 深度拆解：一个最小 Skill 的完整构成

一个完整的 Skill 必须由两部分组成，缺一不可：**本地执行代码**（给计算机看的） + **JSON Schema 说明书**（给 LLM 看的）。

以下是一个完整的最小 Skill 示例（以 Python 为例）：

### 1. 本地执行代码 (Python Method)

这是实实在在干活的逻辑，LLM 本身无法执行这段代码，必须由你的主程序在本地环境调用。

```python
def calculate_frame_checksum(data_bytes: list[int], pid: int) -> str:
    """
    计算底层数据帧的校验和。
    """
    print(f"[Skill Execution] 正在计算 PID 0x{pid:02X} 的校验和...")
    
    # 模拟计算过程
    sum_val = pid
    for byte in data_bytes:
        sum_val += byte
        if sum_val > 0xFF:
            sum_val -= 0xFF
            
    checksum = (~sum_val) & 0xFF
    return f"计算完成：PID 0x{pid:02X} 带有 {len(data_bytes)} 字节数据的校验和为 0x{checksum:02X}"

```

### 2. 工具说明书 (JSON Schema)

这是封装在 API Request 的 `tools` 数组中发送给 LLM 的报文。**LLM 完全依靠这里的 `description` 来决定何时调用这个函数，并依靠 `parameters` 来生成正确的参数提取指令。**

```json
{
  "type": "function",
  "function": {
    "name": "calculate_frame_checksum",
    "description": "当需要验证或生成底层总线通信（如LIN总线或CAN总线）的数据帧校验和时，调用此工具。你需要提供数据字节列表和受保护的ID (PID)。",
    "parameters": {
      "type": "object",
      "properties": {
        "data_bytes": {
          "type": "array",
          "items": {
            "type": "integer"
          },
          "description": "十进制格式的数据字节列表，例如 [25, 68, 12]"
        },
        "pid": {
          "type": "integer",
          "description": "帧的受保护标识符 (PID)，十进制格式"
        }
      },
      "required": ["data_bytes", "pid"]
    }
  }
}

```

## 三、 Agent 的运转机制：ReAct (Reason + Act) 循环

Agent 的本质，就是一个包在 LLM API 外面的 `while` 循环。业界将这种执行逻辑称为 ReAct 模式。

1. **输入指令**：将用户的自然语言问题和所有可用 Skill 的 JSON Schema 组装成报文发给 LLM。
2. **LLM 决策 (Reason)**：LLM 发现解决问题需要外部工具，于是 API 停止生成文本，返回一个状态码（如 `finish_reason: "tool_calls"`），并输出一个包含函数名（如 `calculate_frame_checksum`）和参数的 JSON。
3. **本地执行 (Act)**：Agent 框架（你的 Python 主脚本）捕获该请求，提取参数，执行第一部分的 Python 函数。
4. **状态同步**：Agent 将 Python 函数的 return 结果（字符串）追加到对话历史（Messages Context）中，角色标记为 `tool`。
5. **循环往复**：带着最新的执行结果，Agent 再次请求 LLM。LLM 会根据拿到的校验和结果，继续回答用户的问题。如果还需要其他信息，它会继续触发下一个 Skill；如果信息已足够，API 返回 `finish_reason: "stop"`，循环结束。

---
## 3. MCP 协议 原理
你刚才已经看透了 LLM 的底层交互——无非就是给大模型发送包含 `tools`（工具说明书）和 `messages`（上下文）的 JSON。

**Model Context Protocol (MCP，模型上下文协议)** 正是在这个 JSON 接口的基础之上，诞生的一套“革命性”的架构。

你可以把 MCP 理解为 **AI 领域的“USB Type-C 接口”**。

在解释它之前，我们先回到刚才那个手写的 Python 脚本，看看在没有 MCP 时，我们面临什么痛点。

---

### 一、 痛点：为什么我们需要 MCP？

在我们之前的例子中，你的 Python 脚本（Agent）和你的工具（Skill，比如 `calculate_frame_checksum`）是**紧紧绑死在一起的**。

假设你现在写了 50 个非常棒的自动化测试工具（读日志、查数据库、算校验和）。

* 如果你想在自己写的 Python 脚本里用，你要把这 50 个工具的 JSON Schema 写进代码传给 LLM。
* 如果你想在 **Roo Code** 里面用这 50 个工具，你得去研究 Roo Code 的插件开发机制，重新写一遍接入逻辑。
* 如果明天你想换用 **Cursor** 或者 **Claude Desktop**，你还得再写一遍对接代码。

**每次换一个 AI 应用，你的底层工具库就得重新适配一次。这太低效了。**

### 二、 MCP 是什么？它的功能

为了解决这个问题，Anthropic（Claude 的母公司）牵头推出了 MCP。
**MCP 的核心功能，就是把“AI 应用”和“本地数据/工具”彻底解耦，变成标准的 C/S（客户端/服务端）架构。**

在 MCP 的世界里，存在两个角色：

1. **MCP Host / Client (客户端)**：比如 Roo Code、Cursor、Claude Desktop，甚至是刚才你手写的那个带有 `while` 循环的 Python 主程序。它们负责和 LLM（大模型）对话。
2. **MCP Server (服务端)**：一个独立运行的轻量级本地服务。**你的所有 Skill（Python 函数）和它们的 JSON Schema（工具说明书），统统放在这里面。**

### 三、 MCP 的运行原理（从 LLM 的 JSON 接口讲起）

加入 MCP 后，我们之前梳理的 `ReAct` 循环发生了极其优雅的变化。你可以把 MCP 想象成一个专门负责“搬运 JSON”的中间件。

以下是完整的交互流程：

#### Step 1: 握手与工具发现 (Discovery)

* **以前**：你自己在 Python 代码里硬编码 `tools = [...]`。
* **现在 (MCP)**：Roo Code (Client) 启动时，通过本地进程间通信（如 stdio 或 HTTP）连接你的 MCP Server，发送一个指令：`"喂，把你的说明书交出来！"`。
* MCP Server 把 `calculate_frame_checksum` 的 **JSON Schema** 按照 MCP 协议标准发给 Roo Code。
* 此时，Roo Code 内部就拿到了这个工具的说明书。

#### Step 2: 组装 JSON 请求大模型

* 用户在 Roo Code 里输入：“帮我算一下这帧数据的校验和”。
* Roo Code 把用户的话放入 `messages`，把你 MCP Server 提供的 JSON Schema 放入 `tools`。
* 发起 HTTP 请求（比如调用 OpenAI 或 Ollama 的 `/v1/chat/completions` API）。

#### Step 3: 解析大模型返回的 JSON

* 大模型（LLM）决定调用工具，返回包含 `tool_calls` 的 JSON（指定要调用的函数名和参数）。

#### Step 4: 远程执行工具 (MCP 的核心作用)

* **以前**：你的主程序直接在本地 `if func_name == ...` 然后调用 Python 方法。
* **现在 (MCP)**：Roo Code 本身没有计算校验和的代码。它通过 MCP 协议，给你的 MCP Server 发送一条指令（类似于 RPC 调用）：
`"LLM 让我执行 calculate_frame_checksum，参数是 {...}，你跑一下把结果给我。"`
* 你的 MCP Server 收到后，执行本地真正的 Python 代码，拿到结果字符串，通过 MCP 协议返回给 Roo Code。

#### Step 5: 回传结果给 LLM

* Roo Code 拿到结果，封装成 `role: "tool"` 追加到 `messages` 里，再次请求大模型，完成闭环。

---

### 四、 MCP Server 里到底装了什么？

一个标准的 MCP Server 除了提供 **Tools（工具/也就是我们说的 Skill）** 之外，还可以提供另外两样东西，这构成了它的三大核心能力：

1. **Resources (资源)**：
提供给大模型的静态数据上下文。类似于系统里的只读文件。比如你可以写一个 MCP Server，专门把本地的 API 文档、数据库 Schema 暴露为 Resource。客户端可以随时读取它们作为 LLM 的背景知识。
2. **Prompts (提示词模板)**：
服务端可以预定义好一些复杂的提示词。客户端调用时，直接拉取这些模板。
3. **Tools (工具)**：
就是我们讲的核心——允许 LLM 执行动作（读写文件、发请求、跑代码）。也就是暴露函数定义（JSON Schema）和实际的执行逻辑。

### 五、 总结

把 MCP 掰开了揉碎了看：
**它本质上就是一个标准化的 JSON 搬运工和执行器网关。**

有了 MCP 之后，你只需要用 Python（或者 Node.js）写一个 `My_Hardware_Test_MCP_Server`。在这个 Server 里定义好各种跟底层硬件通信、解析日志的 Skill。
然后，你可以把这个 Server 直接挂载到 Roo Code 里，你的 Roo Code 瞬间就拥有了调试硬件的能力；明天你想用别的 AI IDE，只要它支持 MCP，一键连接，依然完美运行。

---
## 4. MCP个人化理解/补充
这份笔记为你整理好了。采用 Q&A 的形式，将你的核心理解与底层的工程实现细节进行了完美结合，非常适合作为你构建 AI Agent 知识体系的备忘录。

# 学习笔记：MCP (Model Context Protocol) 核心架构与运行机制

### Q1：MCP Server 是不是一个独立运行的 Service？它主要负责向 Client 提供可用的能力列表（如 `tool_schemas`）？

**核心理解：完全正确。**
MCP 的核心思想就是“解耦”。MCP Server 是一个完全独立的后台服务（就像一个底层的驱动模块）。它的首要任务之一，就是在 Client（如 Roo Code、Cursor 等 AI IDE）连接时，告诉 Client 自己拥有哪些工具（返回 `tool_schemas` 的 JSON 描述）。

**🛠️ 工程补充：三大核心能力 (Primitives)**
除了提供 Tools（工具/Skill），一个完整的 MCP Server 还可以提供另外两项核心资产：

1. **Resources (资源)**：向 LLM 暴露本地的静态上下文数据（如本地数据库、API 文档、项目架构图等）。Client 可以像读取文件系统一样，通过特定的 URI 读取这些内容。
2. **Prompts (提示词)**：Server 端可以预设好特定的系统提示词模板，供 Client 直接拉取使用，统一 Agent 的行为规范。

---

### Q2：MCP Server 的执行逻辑，是不是根据 LLM 返回的 JSON 指令（由 Client 转发过来），在本地调用具体的 Skill，然后把结果返回给 Client？

**核心理解：完全正确。**
MCP Server 扮演的是“纯粹的执行者”角色。它本身不具备智能，只是忠实地接收包含函数名和参数的指令，执行对应的本地 Python/Node.js 方法，然后将执行结果（通常是字符串）通过协议回传。

**🔒 工程补充：Client 是绝对的“主设备” (Master)**
在安全性设计上，**大模型 (LLM) 绝对不能直接与 MCP Server 通信**。
所有的控制权都在 Client 手里：

* LLM 生成调用指令后，发给 Client。
* Client 会进行**安全拦截**（例如弹出窗口，询问用户：“AI 试图运行一段 Python 脚本或执行 SQL，是否允许？”）。
* 只有用户（或 Client 的自动放行策略）批准后，Client 才会把指令通过 MCP 协议派发给 Server。Server 只干活，不负责审批。

---

### Q3：Client 和 Service 之间的通信遵循 MCP 协议，是不是意味着只要大家都遵循这套标准，底层的工具模块（Skill）就可以跨不同 AI 框架无限复用？

**核心理解：完全正确。**
这正是 MCP 被称为 AI 领域“USB Type-C 接口”的原因。你只需要用 Python 写一次底层的硬件测试接口、日志分析工具（封装进 MCP Server），就可以零成本地将它们接入 Roo Code、Claude Desktop、Cursor 等任何支持 MCP 的先进前端中。换 AI 工具，再也不需要重写底层代码。

**🔌 工程补充：物理通信层 (Transport) 的两种实现方式**
既然是 C/S 架构，它们之间是通过什么物理通道传输 JSON 报文的呢？主要有两种：

1. **Stdio (标准输入输出)**：**最常用的本地开发模式。** Client（如 Roo Code）在后台直接拉起你的 Python 脚本进程，然后通过进程间的 `stdin` 和 `stdout` 来进行高速的数据收发。极其轻量，无需占用网络端口。
2. **SSE (Server-Sent Events over HTTP)**：**适用于远程部署或团队共享。** 如果你的 MCP Server 部署在远端服务器上，或者想让局域网内的多个同事一起使用，就可以采用这种模式，通过标准的 HTTP 网络协议进行通信。

---

通过这三个 Q&A，你已经掌握了构建现代、可扩展 AI 工具链的最核心架构。

**接下来，我们是趁热打铁，直接用 Python 演示如何手写一个极简的（基于 Stdio 通信的）MCP Server 跑起来看看，还是你想先探讨一下在 Agent 开发中如何设计高质量的系统提示词（System Prompt）？**