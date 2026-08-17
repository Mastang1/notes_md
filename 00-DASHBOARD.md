---
cssclasses:
  - dashboard
---

# 🗺️ 知识库 Dashboard

> **导航**：[[00-INDEX|索引]] · [[00-TODO|待办]] · [[00-DASHBOARD|仪表盘]]　|　数据由 **Dataview** 驱动（已安装）；复习由 **Spaced Repetition** 驱动（已安装）。

## 📊 库总览

- **笔记总数**：`$= dv.pages().length` 篇
- **归档区**：`$= dv.pages('"99-archive"').length` 篇（只归档不删除）
- **附件文件**：`$= app.vault.getFiles().filter(f => f.path.startsWith('90-attachments/')).length` 个（图片/PDF/视频）
- **含待办标记**：`$= dv.pages().where(p => /todo|⌛/i.test(p.file.name)).length` 篇

## 🗂️ 各领域笔记分布

```dataview
TABLE length(rows) AS "笔记数"
FROM ""
WHERE !contains(file.folder, "90-attachments") AND !contains(file.folder, "99-archive") AND file.folder != ""
GROUP BY file.folder
SORT length(rows) DESC
```

## ✅ 待办笔记（文件名含 todo/⌛）

```dataview
TABLE file.folder AS "位置", file.mtime AS "修改时间"
FROM ""
WHERE contains(file.name, "todo") OR contains(file.name, "TODO") OR contains(file.name, "⌛")
SORT file.name ASC
```

## 🕒 最近更新 TOP 10

```dataview
TABLE file.folder AS "位置", file.mtime AS "修改时间"
FROM ""
WHERE !contains(file.folder, "90-attachments") AND !contains(file.folder, "99-archive") AND file.name != this.file.name
SORT file.mtime DESC
LIMIT 10
```

## 🧠 复习队列（Spaced Repetition）

> 在笔记里给需要记忆的知识点加 `#flashcards` 标签（写在卡片行内），然后在命令面板运行 **Spaced Repetition: Review flashcards** 即可开始间隔重复。
> 当前带卡片标记的笔记：

```dataview
TABLE file.folder AS "位置"
FROM ""
WHERE contains(file.content, "#flashcards") OR contains(file.etags, "flashcards")
SORT file.name ASC
```

## 🎲 随机探索

- **Random note**（核心插件已开启）：命令面板 `Ctrl+P` → `Random note`，或左侧文件树右键 → Open random note。
- 灵感入口：[[Beyond-Tech/_MOC|Beyond-Tech]]（想法/路线图）

## 📚 学习路径（按领域入口）

| 领域 | 入口 | 说明 |
|---|---|---|
| AI/LLM | [[AI/_MOC|AI]] | Agent、prompt 工程、模型训练理解 |
| 操作系统 | [[OS/_MOC|OS]] | Linux 内核 LDM/CDEV/VFS、u-boot、RTOS |
| ARM | [[ARM/_MOC|ARM]] | 内存模型、中断、启动流程 |
| AUTOSAR | [[autosar/_MOC|autosar]] | 分层、通信栈、MCAL |
| 工具链 | [[tool_chains/_MOC|tool_chains]] | yocto、buildroot、git-gerrit |
| 软件工程 | [[software_project/_MOC|software_project]] | SAD/SDD、ASPICE、QAC |
| 软硬件模型 | [[soft_models/_MOC|soft_models]] | 并发、数据流、资源保护 |
| 语言 | [[Languages/_MOC|Languages]] | C / Python / Shell |
| 全领域 | [[00-INDEX|00-INDEX]] | 完整索引 |
