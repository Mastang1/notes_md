
## 1. Summay

_**usage
1.非agent模式实现plan等的交互；
2.创建prompt.md记录常用提示词
3.在agent模式下，命令AI提取rule，将实现流程存入.cursorrules文件，并执行测试
4.问题：当因为会话次数增多导致conversation占比比较大，需要启动新的session，此时需要：
a. 将前序会话的工具脚本整理成skills
b. 在cursorrules中加入rule，导出记忆到指定路径，新会话中可以导入**_