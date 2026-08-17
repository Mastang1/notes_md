---
tags:
  - tool_chains
---

# tool_chains · MOC

> 领域：工具链：yocto、buildroot、git-gerrit、WSL

> 本页由结构整理生成，内容自动跟随文件变动（需安装 Dataview 插件后自动渲染）。

```dataview
TABLE file.mtime AS "修改时间"
FROM "tool_chains"
WHERE file.name != this.file.name
SORT file.name ASC
```

### ⭐ 入口笔记（自动更新）

```dataview
LIST
FROM "tool_chains"
WHERE file.name != this.file.name AND regexmatch("^(0[- ]|top|summary|index|outline|dir|base|目录|总|入口|moc)", lowercase(file.name))
SORT file.name ASC
```
