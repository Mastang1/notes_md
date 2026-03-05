
---
## 1. 对齐、占位符添加（可为变量，动态设置占位符个数）

一、f-string（现代且最推荐）
```python

name = "Alice"  
age = 23  
score = 95.5  
  
print(f"{name:<10}{age:<5}{score:<8}")
```
解释一下：

- `<` 表示左对齐（left align）
    
- `10` 表示占 10 个字符宽度
    
- 不够的部分自动补空格
    

输出会像：
```python
Alice     23   95.5  
```

就像拿尺子量过一样工整。