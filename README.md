# Technical Notes Repository

这是一个综合性的技术笔记仓库，涵盖嵌入式系统开发、AI/LLM应用、汽车软件架构、工具链配置等多个技术领域。笔记采用Markdown格式编写，结构清晰，内容深入，适合工程师和技术爱好者参考学习。

## 📁 目录结构

```
notes_md_work/
├── AI/                          # AI与LLM开发相关笔记
│   ├── 0_top_protocol.md        # LLM协议详解（OpenAI vs Ollama）
│   ├── ai_app_project_concept.md # AI应用项目概念
│   ├── MCP.md                   # Model Context Protocol详解
│   ├── model_runners.md         # 模型运行器
│   ├── layers-function/         # AI分层功能架构
│   └── mcp-and-future/          # MCP协议与未来发展
├── autosar/                     # AUTOSAR汽车软件架构
│   ├── autosar_ECU_lalyers.md   # AUTOSAR ECU软件分层详解
│   ├── AS-toolchains summary.md # AUTOSAR工具链总结
│   ├── config_variants.md       # 配置变体
│   ├── Lin_demo.md              # LIN总线示例
│   ├── mcal_requests.md         # MCAL驱动需求
│   └── autosar OS/              # AUTOSAR操作系统
├── EB-tresos-generator/         # EB tresos Studio工具链
│   ├── 0.High-Level Overview.md # 高级概述
│   ├── config_schema_xml.md     # 配置Schema XML
│   ├── dynamic attributes.md    # 动态属性
│   ├── GUI widget.md            # GUI组件
│   ├── 5-1/                     # 第5章第1节
│   ├── 5-2/                     # 第5章第2节
│   ├── 5-3/                     # 第5章第3节
│   ├── 6-config-modules/        # 第6章：配置模块
│   ├── 7-2/                     # 第7章第2节
│   └── plugins/                 # 插件开发
├── Languages/                   # 编程语言笔记
│   ├── c/                       # C语言
│   │   ├── c.md                 # C语言基础
│   │   └── local_variable_memory.md # 局部变量内存
│   └── python/                  # Python语言
│       ├── asyncio--todo.md     # 异步编程
│       ├── coroutine thread thread-pool process.md # 并发编程
│       └── py-template.md       # Python模板
├── tool_chains/                 # 工具链
│   ├── linker.md                # 链接器
│   ├── makefile.md              # Makefile
│   └── git-gerrit/              # Git/Gerrit工作流
├── development-theory/          # 开发理论
│   ├── arm_core&IP.md           # ARM核心与IP
│   ├── MCU-software-layers.md   # MCU软件分层
│   └── specification.md         # 规范文档
├── ethernet/                    # 以太网相关
│   ├── mac_mii_phy.md           # MAC/MII/PHY
│   ├── wireshark.md             # Wireshark使用
│   └── greatwall/               # 长城相关
├── OS/                          # 操作系统
│   └── Event.md                 # 事件机制
├── ARM/                         # ARM架构
│   └── interrupt.md             # 中断处理
├── desktop_apps/                # 桌面应用
│   └── PyQt-NodeGraphQt.md      # PyQt节点图
├── Software Quality Assurance Process/ # 软件质量保证
│   └── top_steps.md             # 顶层步骤
└── 0-layers.md                  # AUTOSAR层级架构图
```

## 🎯 核心主题

### 1. **AUTOSAR汽车软件架构**
- **软件分层**：Application Layer、RTE、BSW（Services Layer、ECU Abstraction Layer、MCAL）
- **通信栈**：COM、PduR、CanIf、Can等模块详解
- **诊断栈**：DCM、DEM、FIM等诊断服务
- **存储栈**：NvM、MemIf、Fee、Fls等存储管理
- **MCAL驱动**：Can、Lin、Adc、Dio、Port等硬件驱动

### 2. **AI/LLM应用开发**
- **LLM协议**：OpenAI API协议与Ollama协议对比
- **Agent开发**：ReAct模式、Skill/Tool定义、JSON Schema规范
- **MCP协议**：Model Context Protocol架构与实现
- **工具调用**：函数调用、参数提取、执行流程

### 3. **EB tresos Studio工具链**
- **插件开发**：Tresos Plugin架构、XDM配置模型
- **代码生成**：Mako模板、Pre-Compile/Post-Build变体
- **配置管理**：ECUC参数配置、动态属性、GUI组件

### 4. **嵌入式系统开发**
- **ARM架构**：中断处理、核心与IP
- **通信协议**：CAN、LIN、以太网
- **操作系统**：AUTOSAR OS、Trampoline OS
- **开发工具**：Makefile、链接器、版本控制

## 🔧 技术栈

| 领域 | 技术/工具 |
|------|-----------|
| **嵌入式开发** | AUTOSAR Classic Platform、MCAL驱动、CAN/LIN通信、ARM Cortex-M |
| **AI/LLM** | OpenAI API、Ollama、MCP协议、Agent框架、LangChain |
| **工具链** | EB tresos Studio、Makefile、Git/Gerrit、Wireshark |
| **编程语言** | C、Python、Shell脚本、XML/JSON |
| **操作系统** | AUTOSAR OS、Linux、实时操作系统 |

## 📚 特色笔记

### **AUTOSAR层级架构图** ([`0-layers.md`](0-layers.md))
使用Mermaid图表展示AUTOSAR规范体系的完整层级架构，包含：
- 顶层架构与方法论（General & Methodology）
- 运行时环境（RTE）
- 系统服务栈（System Services Stack）
- 通信栈（Communication Stack）
- 存储栈（Memory Stack）
- 诊断栈（Diagnostic Stack）
- MCAL IO/Drivers

### **LLM协议详解** ([`AI/0_top_protocol.md`](AI/0_top_protocol.md))
深入分析LLM交互的核心JSON结构：
- Request/Response JSON关键字段详解
- OpenAI协议 vs Ollama协议对比
- Tool Calling机制与JSON Schema
- 流式输出控制与SSE格式

### **MCP协议架构** ([`AI/MCP.md`](AI/MCP.md))
详细解释Model Context Protocol：
- MCP Server/Client架构
- 工具发现与执行机制
- Stdio与HTTP通信方式
- Resources、Prompts、Tools三大核心能力

### **EB tresos插件开发** ([`EB-tresos-generator/0.High-Level Overview.md`](EB-tresos-generator/0.High-Level Overview.md))
完整的Tresos Plugin开发指南：
- 插件目录结构与核心文件
- XDM配置模型定义
- Mako代码生成模板
- 安装与调试流程

## 🚀 使用说明

### 浏览笔记
1. 使用目录结构导航到感兴趣的主题
2. 查看Markdown文件获取详细技术内容
3. 利用Mermaid图表理解复杂架构

### 搜索内容
- 使用GitHub的搜索功能查找特定关键词
- 按目录结构查找相关主题
- 查看文件间的引用链接

### 贡献与更新
1. 如需添加新笔记，请按现有目录结构组织
2. 使用清晰的Markdown格式编写
3. 添加适当的图表和代码示例
4. 保持技术准确性和实用性

## 📖 学习路径建议

### 对于嵌入式工程师
1. 从 [`0-layers.md`](0-layers.md) 开始了解AUTOSAR整体架构
2. 学习 [`autosar/autosar_ECU_lalyers.md`](autosar/autosar_ECU_lalyers.md) 理解软件分层
3. 查看 [`EB-tresos-generator/`](EB-tresos-generator/) 学习工具链使用

### 对于AI/LLM开发者
1. 阅读 [`AI/0_top_protocol.md`](AI/0_top_protocol.md) 掌握LLM协议基础
2. 学习 [`AI/MCP.md`](AI/MCP.md) 了解MCP架构
3. 查看 [`AI/layers-function/`](AI/layers-function/) 理解AI应用分层

### 对于全栈开发者
1. 浏览 [`Languages/`](Languages/) 学习编程语言技巧
2. 查看 [`tool_chains/`](tool_chains/) 掌握开发工具
3. 学习 [`development-theory/`](development-theory/) 理解开发理论

## 🔗 相关资源

- **AUTOSAR官方文档**：https://www.autosar.org/
- **EB tresos Studio**：https://www.elektrobit.com/products/eb-tresos/
- **OpenAI API文档**：https://platform.openai.com/docs/api-reference
- **Ollama文档**：https://github.com/ollama/ollama
- **MCP协议规范**：https://spec.modelcontextprotocol.io/

## 📄 许可证

本仓库内容为技术笔记，遵循知识共享原则。具体使用请参考 [`About-license.md`](About-license.md)。

## 🤝 贡献

欢迎提交Issue和Pull Request来完善笔记内容。请确保：
1. 技术内容准确可靠
2. 格式符合现有规范
3. 添加适当的图表和示例

---

*最后更新：2026-03-05*  
*维护者：嵌入式与AI技术爱好者*
