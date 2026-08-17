# ✅ TODO 总览

> 自动列出文件名/内容含待办标记的笔记（⌛ ☁️ todo）。需安装 Dataview。

```dataview
TABLE file.folder AS "位置", file.mtime AS "修改时间"
FROM ""
WHERE contains(file.name, "todo") OR contains(file.name, "TODO") OR contains(file.name, "⌛")
SORT file.name ASC
```

### 📌 已知待办笔记

- [[单核共享资源保护策略(todo-summarize)]]
- [[自旋锁与总线嗅探器(todo 个人笔记)]]
- [[asyncio-todo]]
- [[0LDM-short description todo]]
- [[1. wait_queue(todo SELECT POLL EPOLL)]]
- [[1. Subsystem-UIO(todo-深入分析子设备分配 路由机制)]]
- [[poll-A Choice For File(todo分析不到位)]]
- [[5-create_device-1 剖析-基本流程+回旋镖todo]]
- [[5-create_device-2 剖析-为什么 回旋镖todo]]
- [[1.3 module_init 机制 build机制(TODO)]]
- [[Learning Tasks(TODO)]]
- [[mm-newlib-analysis(todo]]