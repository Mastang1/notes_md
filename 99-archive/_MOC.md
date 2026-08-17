---
tags:
  - archive
status: archived
---

# 99-archive · 归档区

> 归档 ≠ 删除：此处笔记保留原内容，仅从工作区移出。

```dataview
TABLE file.mtime AS "归档时间"
FROM "99-archive"
SORT file.name ASC
```

### 📄 清单（自动更新）

```dataview
LIST
FROM "99-archive"
WHERE file.name != this.file.name
SORT file.name ASC
```
