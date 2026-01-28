
## 第 2 层：API 协议层（OpenAI Compatible API）

### 是什么

> **工业级“请求格式标准”**

它解决的问题只有一个：

> **“应用和模型服务之间，怎么说话”**

### 核心特征

- HTTP 接口
    
- JSON 数据结构
    
- role / content 语义
    

示例（本质）：

```json
{
  "model": "qwen3",
  "messages": [
    {"role": "user", "content": "你好"}
  ]
}
```

> **OpenAI 协议 ≈ USB 协议，不是 CPU**

---
