这是一份关于 `python-docx` 的核心功能速查笔记及实战代码示例。

### 📑 `python-docx` 核心学习笔记

**库简介**：一个用于读取、查询和修改 Microsoft Word 2007+ (`.docx` 文件) 的 Python 库。本质是对 Office Open XML (OOXML) 协议的封装。

#### 🛠️ 核心功能速查

1. **文档生命周期**：创建空白文档、加载现有模板文档、保存到本地或内存流。
    
2. **段落与格式 (Paragraph & Run)**：
    
    - 插入各级标题 (`add_heading`) 和正文段落 (`add_paragraph`)。
        
    - 通过 `Run` 对象实现细粒度控制：加粗、斜体、下划线、字体名称、颜色、字号。
        
    - 设置段落对齐方式、行距、缩进。
        
3. **表格操作 (Table)**：
    
    - 动态创建表格 (`add_table`)，自由指定行列数。
        
    - 定位单元格 (`cell(row, col)`) 进行读写，支持合并/拆分单元格。
        
    - 应用 Word 内置的表格样式（如网格型、底纹型）。
        
4. **图像处理 (Picture)**：
    
    - 插入本地图片或二进制图片流 (`add_picture`)。
        
    - **按比例智能缩放**：仅需指定宽度 (`width`) 或高度 (`height`) 其中一项，库会自动保持原图宽高比。
        
5. **页面排版 (Section)**：设置纸张大小（A4等）、页面方向（横向/纵向）、上下左右页边距。
    
6. **底层突破 (Oxml API)**：对于库尚未原生提供的高级高层 API 功能（如特定单元格边框修改、插入目录域代码），可通过 `docx.oxml` 直接操作底层 XML 节点实现。
    

---

### 💻 综合实战 Demo

以下脚本展示了一个标准的文档生成工作流，涵盖了大部分高频操作：

Python

```python
from docx import Document
from docx.shared import Cm, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH

def create_demo_doc():
    # 1. 初始化（新建空白文档）
    doc = Document()

    # 2. 插入标题并居中对齐
    title = doc.add_heading('系统测试报告', level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER

    # 3. 插入正文段落及字符级格式化 (Run)
    p = doc.add_paragraph('本项目测试环境已部署完成。以下是')
    
    # 针对部分文本进行特殊格式化
    run_bold = p.add_run('核心组件')
    run_bold.bold = True
    run_bold.font.color.rgb = RGBColor(255, 0, 0) # 设置为红色
    
    p.add_run('的运行状态摘要：')

    # 4. 插入表格 (2行3列) 并填充数据
    records = (
        ('BSP 层', '版本 v1.2', '正常'),
        ('HAL 层', '版本 v2.0', '警告')
    )
    table = doc.add_table(rows=1, cols=3, style='Table Grid') # 使用网格样式
    
    # 设置表头
    hdr_cells = table.rows[0].cells
    hdr_cells[0].text = '模块'
    hdr_cells[1].text = '版本'
    hdr_cells[2].text = '状态'
    
    # 遍历添加数据行
    for module, version, status in records:
        row_cells = table.add_row().cells
        row_cells[0].text = module
        row_cells[1].text = version
        row_cells[2].text = status

    # 5. 插入插图 (自动保持宽高比)
    # 注意：运行此代码前需确保同目录下有 'architecture.png' 文件
    try:
        doc.add_paragraph('\n附图：系统架构拓扑图')
        # 仅指定宽度为 12 厘米，高度会自动等比缩放
        doc.add_picture('architecture.png', width=Cm(12))
    except FileNotFoundError:
        doc.add_paragraph('[错误提示：未找到 architecture.png 图片文件]', style='Intense Quote')

    # 6. 保存文档
    output_name = 'Demo_Report.docx'
    doc.save(output_name)
    print(f"✅ 文档生成成功，已保存为：{output_name}")

if __name__ == '__main__':
    create_demo_doc()
```