[DeepSeek]
api_key = sk-3aafb4ba07b349dba16b22911255cd6a

请设计一个本地prompt优化输出的CLI工具：

【功能目标】
1. 总体功能：提供命令行交互工具，user输入原始prompt，工具会智能检索本地的prompt库，拉取可能匹配的提示词列表形成 “原始提示词” +"预选的本地提示词列表" ，并加上一个用于让LLM决策、优化最终提示词的逻辑文本，形成一个 “提示词优化”提示词，然后给到deepseek，它的key是“sk-3aafb4ba07b349dba16b22911255cd6a”；然后返回最终提示词；当前会话结束；
2. 命令行工具提供帮助信息
【工程结构】

	prompt_optimization_assistang.py
			prompt_list
					prompt_project_development
						...	
【要求】
- Python实现
- 模块化设计
输出：
1. 架构设计
2. 模块划分
3. 示例代码