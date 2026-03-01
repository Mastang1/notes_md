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
## 3. 