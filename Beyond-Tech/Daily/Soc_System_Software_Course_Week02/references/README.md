# Local Reference Layout

本目录用于保证每日教程里的本地 PDF 链接可直接打开。

当前包已包含用户上传的：

```text
Explorer STM32F4_V2.2_SCH.pdf
```

请把你电脑里的以下资料复制到本目录，**文件名保持一致**：

```text
【正点原子】I.MX6U嵌入式Linux C应用编程指南V1.1.pdf
【正点原子】I.MX6U嵌入式Linux驱动开发指南V1.5.2.pdf
【正点原子】I.MX6U网络环境TFTP&NFS搭建手册V1.3.1.pdf
```

正点原子公开归档仓库可找到这些版本：

- https://github.com/alientek-openedv/imx6ull-document

## PDF 指定页链接

教程采用：

```markdown
[打开原理图 p.2](Explorer%20STM32F4_V2.2_SCH.pdf#page=2)
```

`#page=N` 是否自动跳页取决于 VS Code/Typora/浏览器 PDF viewer，因此每处引用仍会写明“文档名 + 章节/页码 + 阅读目标”。**页码文本是权威定位，超链接只是便利。**


## Recommended English aliases for local PDFs

为避免 Windows/压缩软件对中文文件名处理不一致，后续本地大 PDF 建议重命名为 ASCII 文件名后放入本目录：

```text
IMX6ULL_Linux_C_Application_Guide_V1.1.pdf
IMX6ULL_Linux_Driver_Development_Guide_V1.5.2.pdf
IMX6ULL_TFTP_NFS_Setup_Guide_V1.3.1.pdf
```

`source_index.md` 仍保留原书中文标题用于识别，但课程文件和压缩包内文件名一律使用英文/ASCII。
