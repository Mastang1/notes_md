太棒了！接下来我们将进入真正的实战阶段。

为了让你能够完美掌控 LangGraph，并最终实现你的 STM32 自动化开发助手，我为你精心设计了 **6 个递进式的核心 Demo**（精于质而非量，这 6 个足以涵盖所有核心架构）。

我们会模拟 STM32 的开发流程（写代码 -> 编译 -> 报错重写 -> 确认烧录）。

### 准备工作：环境与配置

首先，确保安装了必要的库。因为 DeepSeek 的 API 是完全兼容 OpenAI 接口的，所以我们直接使用 LangChain 的 OpenAI 模块，只需修改 `base_url` 即可。

Bash

```
pip install langgraph langchain-openai
```

在你的代码同级目录下，创建一个 `config.json`：

JSON

```
{
    "DEEPSEEK_API_KEY": "sk-你的真实DeepSeekAPIKey"
}
```

---

### Demo 1: 最简图结构与 LLM 接入 (Hello World)

**【学习目标】**：理解 LangGraph 的三大基石：`State` (状态/上下文)、`Node` (节点/任务)、`Edge` (边/流程)。

**【心法记忆】**：Graph 就像一个状态机。你必须先定义一个“数据结构(State)”，然后让各个函数(Node)去修改这个数据，最后连线(Edge)。

Python

```
import json
from typing import TypedDict
from langgraph.graph import StateGraph, START, END
from langchain_openai import ChatOpenAI

# ================= 辅助函数：读取 API Key =================
def get_llm():
    with open('config.json', 'r') as f:
        config = json.load(f)
    # 记忆点：DeepSeek 兼容 OpenAI 格式，直接指定 base_url 即可
    return ChatOpenAI(
        api_key=config["DEEPSEEK_API_KEY"], 
        base_url="https://api.deepseek.com", 
        model="deepseek-chat"
    )

# 1. 定义 State (状态)。就像是 RTOS 里的全局结构体，在所有节点间传递
class AgentState(TypedDict):
    messages: list[str] # 记录对话历史或当前指令

# 2. 定义 Node (节点)。节点本质上就是一个接收 State，并返回**更新后 State** 的 Python 函数
def call_deepseek(state: AgentState):
    llm = get_llm()
    user_input = state["messages"][-1]
    # 告诉模型它的身份
    response = llm.invoke([
        ("system", "你是一个资深的 STM32 嵌入式工程师。"),
        ("user", user_input)
    ])
    # 返回的内容会自动追加/覆盖到 State 中
    return {"messages": state["messages"] + [response.content]}

# 3. 构建 Graph (状态机连线)
# 记忆点：StateGraph(状态结构体) -> add_node(名字, 函数) -> add_edge(起点, 终点) -> compile(编译生效)
builder = StateGraph(AgentState)
builder.add_node("expert_node", call_deepseek)

# 流程：START开始 -> 专家节点 -> END结束
builder.add_edge(START, "expert_node")
builder.add_edge("expert_node", END)

# 4. 编译并运行
graph = builder.compile()

if __name__ == "__main__":
    # 初始状态输入
    initial_state = {"messages": ["请用一句话解释什么是 STM32 的 GPIO 浮空输入？"]}
    # invoke 触发图执行
    result = graph.invoke(initial_state)
    print("最终结果:\n", result["messages"][-1])
```

---

### Demo 2: 多节点协作与状态流转 (The Pipeline)

**【学习目标】**：掌握如何在多个 Agent/Node 之间传递复杂数据。

**【心法记忆】**：State 里的字段可以自由定义。前一个节点的输出，就是后一个节点的输入。

Python

```
import json
from typing import TypedDict
from langgraph.graph import StateGraph, START, END
from langchain_openai import ChatOpenAI

def get_llm():
    config = json.load(open('config.json', 'r'))
    return ChatOpenAI(api_key=config["DEEPSEEK_API_KEY"], base_url="https://api.deepseek.com", model="deepseek-chat")

# 1. 定义更复杂的 State：包含需求描述、架构思路、最终代码
class DevState(TypedDict):
    requirement: str
    architecture: str
    c_code: str

# 2. 节点 A：架构师（负责分析外设和寄存器）
def architect_node(state: DevState):
    llm = get_llm()
    prompt = f"针对需求：{state['requirement']}。请列出需要配置的 STM32 外设和大致步骤（限100字）。"
    response = llm.invoke(prompt)
    return {"architecture": response.content} # 更新 architecture 字段

# 3. 节点 B：码农（根据架构写代码）
def coder_node(state: DevState):
    llm = get_llm()
    prompt = f"架构设计：{state['architecture']}。请写出简短的 C 语言初始化函数。"
    response = llm.invoke(prompt)
    return {"c_code": response.content} # 更新 c_code 字段

# 4. 组装流水线 (Pipeline)
builder = StateGraph(DevState)
builder.add_node("architect", architect_node)
builder.add_node("coder", coder_node)

# 流转顺序：START -> 架构师 -> 码农 -> END
builder.add_edge(START, "architect")
builder.add_edge("architect", "coder")
builder.add_edge("coder", END)

graph = builder.compile()

if __name__ == "__main__":
    res = graph.invoke({"requirement": "配置 PA9 为串口 TX 引脚"})
    print("【架构思路】:\n", res["architecture"])
    print("\n【最终代码】:\n", res["c_code"])
```

---

### Demo 3: 条件路由 (Conditional Edges) - 系统的灵魂

**【学习目标】**：让 Agent 拥有判断力，能够根据状态决定下一步走哪里。

**【心法记忆】**：`add_conditional_edges` 是控制流的分支（if-else）。你需要提供一个路由函数，根据它的返回值决定去哪个节点。

Python

```
import json
from typing import TypedDict
from langgraph.graph import StateGraph, START, END
from langchain_openai import ChatOpenAI

class CheckState(TypedDict):
    code: str
    has_error: bool
    error_msg: str

# 模拟编译工具节点
def mock_compiler(state: CheckState):
    code = state["code"]
    print(f"--- 正在编译代码 ---")
    # 模拟编译检查：如果不包含 "HAL_Init"，就报错
    if "HAL_Init" not in code:
         return {"has_error": True, "error_msg": "Error: 未调用 HAL_Init()"}
    return {"has_error": False, "error_msg": "Success"}

# 路由函数：根据 State 决定走向
# 记忆点：返回值是一个字符串，对应目标节点的名字
def compiler_router(state: CheckState):
    if state["has_error"]:
        return "rewrite_node" # 报错就去重写
    else:
        return END # 成功就结束

# 重写节点
def rewrite_node(state: CheckState):
    print(f"--- 触发重写，因为：{state['error_msg']} ---")
    return {"code": "int main() { HAL_Init(); return 0; }", "has_error": False}

builder = StateGraph(CheckState)
builder.add_node("compiler", mock_compiler)
builder.add_node("rewrite_node", rewrite_node)

builder.add_edge(START, "compiler")
# 核心：添加条件边。起点是 compiler，路由函数是 compiler_router
builder.add_conditional_edges("compiler", compiler_router)
# 重写完后，再次送回编译器检查（这就形成了循环基础）
builder.add_edge("rewrite_node", "compiler")

graph = builder.compile()

if __name__ == "__main__":
    # 初始代码故意写错
    res = graph.invoke({"code": "int main() { return 0; }", "has_error": False})
    print("\n最终输出代码:\n", res["code"])
```

---

### Demo 4: 工具调用 (Tool Calling) - 赋予操作硬件的能力

**【学习目标】**：让 LLM 决定调用 Python 函数（比如执行 gcc 编译）。

**【心法记忆】**：使用 `@tool` 装饰器定义工具。使用 `llm.bind_tools([tools])` 告诉模型有哪些工具可用。

Python

```
import json
from typing import Annotated, TypedDict
from langchain_core.tools import tool
from langgraph.graph import StateGraph, START, END
from langgraph.graph.message import add_messages
from langgraph.prebuilt import ToolNode, tools_condition
from langchain_openai import ChatOpenAI

def get_llm():
    config = json.load(open('config.json', 'r'))
    return ChatOpenAI(api_key=config["DEEPSEEK_API_KEY"], base_url="https://api.deepseek.com", model="deepseek-chat")

# 1. 定义工具 (Tools)。函数的 Docstring 非常重要，大模型靠它理解工具的作用！
@tool
def execute_gcc(c_file_name: str) -> str:
    """调用底层的 arm-none-eabi-gcc 编译指定的 C 文件"""
    # 模拟真实执行环境
    print(f"\n[Tool Execution] 正在运行 gcc 编译 {c_file_name}...")
    return f"Success: {c_file_name} compiled to .elf successfully."

tools = [execute_gcc]

# 2. State 定义：使用 add_messages 自动合并对话历史
class AgentState(TypedDict):
    messages: Annotated[list, add_messages]

# 3. Agent 节点
def agent_node(state: AgentState):
    llm = get_llm()
    # 绑定工具！这是让模型拥有能力的关键
    llm_with_tools = llm.bind_tools(tools)
    response = llm_with_tools.invoke(state["messages"])
    return {"messages": [response]}

builder = StateGraph(AgentState)
builder.add_node("agent", agent_node)
# LangGraph 预置了 ToolNode，可以直接把工具列表传进去，它会自动执行模型请求的工具
builder.add_node("tools", ToolNode(tools))

builder.add_edge(START, "agent")
# 记忆点：tools_condition 会检查模型的回复，如果模型调用了工具，就去 "tools" 节点，否则结束
builder.add_conditional_edges("agent", tools_condition)
builder.add_edge("tools", "agent") # 工具执行完，把结果返回给模型继续判断

graph = builder.compile()

if __name__ == "__main__":
    # 我们直接让模型去编译 main.c
    res = graph.invoke({"messages": [("user", "帮我用 gcc 编译一下 main.c 文件。")]})
    print("\n最终对话:\n", res["messages"][-1].content)
```

---

### Demo 5: 完整的 STM32 自动纠错闭环 (Cyclic Debugging)

**【学习目标】**：将前面所有的知识融合，实现：“写代码 -> 编译验证 -> 提取报错反馈给模型 -> 修改代码 -> 编译通过”。

**【心法记忆】**：多 Agent 循环的核心就在于状态流转和精确的判断。这是你未来开发的核心架构骨架。

Python

```
import json
from typing import TypedDict
from langgraph.graph import StateGraph, START, END
from langchain_openai import ChatOpenAI

class STM32State(TypedDict):
    task: str
    code: str
    compile_log: str
    iterations: int # 防止死循环的安全计数器

def generate_code_node(state: STM32State):
    print(f"--- [Node] 生成/修改代码 (第 {state['iterations'] + 1} 次) ---")
    llm = json.load(open('config.json', 'r'))
    chat = ChatOpenAI(api_key=llm["DEEPSEEK_API_KEY"], base_url="https://api.deepseek.com", model="deepseek-chat")
    
    prompt = f"任务：{state['task']}。\n"
    if state['compile_log']:
        prompt += f"\n上次编译报错：{state['compile_log']}\n请修正代码，仅输出 C 代码主体。"
    else:
        prompt += "请生成 C 代码实现。"
        
    response = chat.invoke(prompt)
    return {"code": response.content, "iterations": state['iterations'] + 1}

def compile_test_node(state: STM32State):
    print("--- [Node] 交叉编译测试 ---")
    code = state['code']
    # 模拟真实的 gcc 报错：如果缺少 #include "stm32f10x.h" 就报错
    if "stm32f10x.h" not in code:
        log = "error: 'GPIO_InitTypeDef' undeclared (first use in this function)"
    else:
        log = "SUCCESS"
    return {"compile_log": log}

def router_check(state: STM32State):
    if state["compile_log"] == "SUCCESS":
        return END
    if state["iterations"] >= 3:
        print("--- [Router] 超过最大重试次数，强制退出 ---")
        return END
    return "generate_code" # 报错就回去重新生成

builder = StateGraph(STM32State)
builder.add_node("generate_code", generate_code_node)
builder.add_node("compile_test", compile_test_node)

builder.add_edge(START, "generate_code")
builder.add_edge("generate_code", "compile_test")
builder.add_conditional_edges("compile_test", router_check)

graph = builder.compile()

if __name__ == "__main__":
    init_state = {"task": "初始化 GPIOA Pin 0 为推挽输出", "code": "", "compile_log": "", "iterations": 0}
    res = graph.invoke(init_state)
    print("\n✅ 最终完美代码:\n", res['code'])
```

---

### Demo 6: 烧录前的“人类介入” (Human-in-the-Loop)

**【学习目标】**：硬件操作不可逆（如烧写 Flash），如何在关键节点前让程序暂停，等待你的回车确认再继续？

**【心法记忆】**：引入 `MemorySaver` (记忆存储) 并使用 `interrupt_before` 参数。

Python

```
from langgraph.graph import StateGraph, START, END
from langgraph.checkpoint.memory import MemorySaver # 记忆库，用于保存暂停时的状态
from typing import TypedDict

class FlashState(TypedDict):
    firmware_path: str
    is_flashed: bool

def build_firmware(state: FlashState):
    print("-> 固件打包完成:", state["firmware_path"])
    return state

def flash_mcu(state: FlashState):
    print(f"-> 正在调用 OpenOCD 将 {state['firmware_path']} 烧录进 STM32F103...")
    return {"is_flashed": True}

builder = StateGraph(FlashState)
builder.add_node("build", build_firmware)
builder.add_node("flash", flash_mcu)

builder.add_edge(START, "build")
builder.add_edge("build", "flash")
builder.add_edge("flash", END)

# 核心：配置 Checkpointer，并在 "flash" 节点之前中断！
memory = MemorySaver()
graph = builder.compile(
    checkpointer=memory,
    interrupt_before=["flash"] # 拦截在此节点之前
)

if __name__ == "__main__":
    # 提供一个 thread_id，类似于开启一个会话进程
    config = {"configurable": {"thread_id": "stm32_debug_session_1"}}
    
    # 第一次运行：执行 build，然后在 flash 前暂停
    print("【第一阶段执行】")
    state = graph.invoke({"firmware_path": "build/main.hex", "is_flashed": False}, config)
    
    print("\n⚠️ 流程已暂停，等待人类确认烧录...")
    # 查看当前中断状态
    snapshot = graph.get_state(config)
    print("下一步准备执行的节点:", snapshot.next)
    
    user_input = input("输入 'y' 确认烧写物理芯片: ")
    if user_input.lower() == 'y':
        # 第二次运行：传入 None，让图从断点继续往下走
        print("\n【第二阶段执行】")
        final_state = graph.invoke(None, config)
        print("烧录结果:", final_state["is_flashed"])
```

---

### 你的下一步战略建议：

这 6 个 Demo 涵盖了你开发 STM32 Agent **95%** 的框架知识。

建议你先将 **Demo 4 (工具调用)** 和 **Demo 5 (循环纠错)** 运行起来。接下来，你只需要把你日常在终端里敲的 `arm-none-eabi-gcc ...` 和 `openocd ...` 命令，包装成 Demo 4 里的 `@tool` 函数，然后接入 Demo 5 的循环里，你的【STM32 自动化 AI 调试助手 V1.0】就初具雏形了！

运行过程中有任何报错，或者不知道怎么结合本地的工程目录，随时贴给我看。