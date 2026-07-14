### 📝 编译器“强制包含” (Pre-include) 高级用法笔记

**📌 核心思想**

无需在代码中手动写 `#include`，通过编译器参数，**强制在所有 `.c` 源文件编译的最开始悄悄注入一个头文件**。

这是“无侵入式覆盖底层 `#ifndef` 默认宏”的最优工程实践。

**⚙️ 工具链配置速查表**

|**编译器/IDE**|**配置方法**|**参数/填写位置**|
|---|---|---|
|**GCC / Makefile / CMake**|增加 `-include` 参数（注意是小写 `i`）|`CFLAGS += -include app_config.h`|
|**Keil MDK (ARM Compiler)**|增加 `--preinclude` 参数|魔术棒 -> C/C++ -> Misc Controls 填入：`--preinclude="app_config.h"`|
|**IAR EWARM**|使用图形化选项|Project Options -> C/C++ Compiler -> Preprocessor -> **Preinclude file** 填入文件名|

**💡 为什么专家都爱用它？**

- **绝对优先**：等效于在所有文件第 0 行插入代码，完美抢占 `#ifndef` 先机。
    
- **零侵入性**：升级第三方中间件或 RTOS 源码时，原厂代码一行不用改，配置依然生效。
    
- **全局掌控**：所有模块共享同一份配置，告别因为 Include 包含顺序错乱导致的“宏未生效”奇葩 Bug。