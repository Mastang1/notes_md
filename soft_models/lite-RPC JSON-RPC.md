你的 `argc/argv` 实现非常经典，这在计算资源受限的嵌入式开发中是一种极具实用价值的优化策略，摒弃了语法树构建的开销。我将这两种方案的对比、设计思路及实现流程整理为适合你直接归档的笔记结构。

  

## 架构对比分析

|**维度**|**Lite-RPC (你的实现)**|**JSON-RPC**|
|---|---|---|
|**序列化格式**|纯字符串，特定分隔符（如 `,`）|标准 JSON 格式字符串|
|**解析开销**|**极低**。仅需 `strtok` 等原位指针偏移|**较高**。需动态分配内存构建语法树|
|**类型安全**|**弱**。全靠 Server 端底层函数执行 `atoi`/`atof` 自行转换|**中**。自带 Number/String/Bool 等基础类型定义|
|**复杂结构**|难以传递嵌套结构体或数组|天然支持多层级嵌套对象|
|**适用场景**|内存与算力受限的 MCU/Edge 设备，固定简单指令|跨语言、跨平台网关或上位机通信|

## 核心设计笔记

- **共性思路**：两者均遵循完整的 RPC 代理模型。客户端隐藏传输细节（Client Stub），服务端负责反序列化与查表路由（Server Stub）。
    
      
    
- **路由机制**：均依赖“字符串 -> 函数指针”的映射注册表。
    
      
    
- **Lite-RPC 特有优势**：参数长度动态识别（`argc`），内存占用确定且极小，无频繁 `malloc/free` 导致的内存碎片风险，非常适合硬实时环境。
    
      
    

## 伪代码实现模型

C

```c
// ==================== 1. Lite-RPC 实现模型 ====================
// Server 端
RETURN_VALUE execute_rpc(char *payload) {
    char *argv[10]; int argc = 0;
    char *func_name = strtok(payload, ",");
    while((argv[argc] = strtok(NULL, ",")) != NULL) argc++;
    
    func_ptr target_func = lookup_registry(func_name);
    return target_func(argv, argc); // 底层函数内部自行 atoi(argv[0])
}

// Client 端
RETURN_VALUE call_lite_rpc(const char *func, const char *arg1, const char *arg2) {
    char buf[128];
    sprintf(buf, "%s,%s,%s", func, arg1, arg2);
    char *response = send_and_receive(buf);
    return parse_return_value(response);
}

// ==================== 2. JSON-RPC 实现模型 ====================
// Server 端
char* execute_json_rpc(const char *json_str) {
    cJSON *req = cJSON_Parse(json_str);
    char *func_name = req->method;
    cJSON *params = req->params; // 树节点
    
    wrapper_func target = lookup_registry(func_name);
    cJSON *result = target(params); // Wrapper内部提取数字并调用纯C函数
    return cJSON_Print(result);
}

// Client 端
int call_json_rpc(const char *func, int arg1) {
    cJSON *req = create_json_request(func, arg1);
    char *response = send_and_receive(cJSON_Print(req));
    return extract_json_result(response);
}
```

## 时序图全流程

以下为两者的时序逻辑，Lite-RPC 省略了解析树的构建，更为扁平。

代码段

![[Pasted image 20260907135646.png]]