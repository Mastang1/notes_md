
## 第 3 层：模型运行器（Ollama）【重点】


> ==**Ollama = 本地大模型 HTTP 服务器 + 推理运行时**==

### Ollama 内部真实结构（非常关键）

你可以把 Ollama 拆成 6 个子模块：

|子模块|作用|
|---|---|
|HTTP Server|接收 OpenAI API 请求|
|Request Parser|解析 JSON|
|Tokenizer|文本 → token|
|Model Loader|加载 Qwen3 权重|
|Inference Engine|执行模型推理|
|Detokenizer|token → 文本|

---

### Ollama 实际干的完整流程

#### 1️⃣ 接收请求

```http
POST /v1/chat/completions
```

- 来自 Dify
    
- 格式符合 OpenAI API
    

---

#### 2️⃣ 解析请求

Ollama 读取：

- model 名称
    
- messages 内容
    
- temperature / max_tokens 等参数
    

---

#### 3️⃣ 调用 Tokenizer（关键）

```
"你好"
↓
[31245, 9981]
```

要点：

- tokenizer **和模型强绑定**
    
- Qwen3 用的是 Qwen3 tokenizer
    
- token 是整数 ID，不是字符串
    

---

#### 4️⃣ 加载模型权重

- `.gguf` / `.bin` 权重文件
    
- 通常来自 HuggingFace
    
- 可能是量化版本（Q4/Q8）
    

> **模型本体 ≈ 巨大的参数矩阵**

---

#### 5️⃣ 推理（Inference）

模型开始循环：

```
已知 token[0..n]
→ 预测 token[n+1]
```

- attention
    
- transformer
    
- GPU / CPU / AVX / CUDA
    

⚠️ **这是唯一“AI”的地方**

---

#### 6️⃣ 反 Token（Detokenize）

```
[456, 9812, 77]
↓
"很高兴见到你"
```

---

#### 7️⃣ 封装返回 JSON

```json
{
  "choices": [
    {
      "message": {
        "role": "assistant",
        "content": "很高兴见到你"
      }
    }
  ]
}
```

---
