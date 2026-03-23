
# 1. 设置 API Key
$env:OPENAI_API_KEY="sk-您的DeepSeek的Key"

# 2. 设置 Base URL，将 OpenAI 的请求重定向到 DeepSeek
$env:OPENAI_BASE_URL="https://api.deepseek.com/v1"

# 3. 再次运行您的翻译命令
pdf2zh in.pdf -s openai:deepseek-chat