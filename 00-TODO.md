# ✅ TODO 总览

> 自动列出文件名含待办标记的笔记（todo / ⌛）。由 Dataview 实时生成，无需手动维护。

```dataview
TABLE file.folder AS "位置", file.mtime AS "修改时间"
FROM ""
WHERE contains(file.name, "todo") OR contains(file.name, "TODO") OR contains(file.name, "⌛")
SORT file.name ASC
```
