# test_etc · MOC

> 领域：测试相关

> 本页由结构整理生成，内容自动跟随文件变动（需安装 Dataview 插件后自动渲染）。

```dataview
TABLE file.mtime AS "修改时间"
FROM "test_etc"
WHERE file.name != this.file.name
SORT file.name ASC
```
