---
tags:
  - asr_project
---

# asr_project · MOC

> 领域：语音识别项目：sherpa-onnx、KWS、部署与方案

> 本页由结构整理生成，内容自动跟随文件变动（需安装 Dataview 插件后自动渲染）。

```dataview
TABLE file.mtime AS "修改时间"
FROM "asr_project"
WHERE file.name != this.file.name
SORT file.name ASC
```

### ⭐ 入口笔记（自动更新）

```dataview
LIST
FROM "asr_project"
WHERE file.name != this.file.name AND regexmatch("^(0[- ]|top|summary|index|outline|dir|base|目录|总|入口|moc)", lowercase(file.name))
SORT file.name ASC
```
