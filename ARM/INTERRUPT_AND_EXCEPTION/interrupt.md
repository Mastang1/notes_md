
# 1. interrupt

这是一份针对前面讨论内容的工程笔记总结，以及一个具备“现场保护”和“调试辅助”功能的 `Default_Handler` 实现方案。

---

# 嵌入式开发笔记：64位数据原子性与中断管理

## 1. 32位MCU上的64位数据处理

在32位架构（如ARM Cortex-M）上，处理64位数据（`uint64_t`）不是单指令完成的，而是拆分为两个32位指令。

- **风险点（Race Condition）：**如果在读取低32位和高32位之间发生了中断，且中断修改了该变量，会导致数据撕裂（Tearing）。
    
- **解决方案：** 必须使用临界区保护。
    
    - **写操作：** `__disable_irq()` -> 赋值 -> `__enable_irq()`。
        
    - **移位操作：** 必须强制类型转换，例如 `((uint64_t)1 << 32)`，且字面量加 `ULL` 后缀。
        
    - **联合体（Union）：** 慎用，严禁用于跨平台序列化，受大小端（Endianness）影响极大。
        

## 2. 临界区与中断丢失

关于“关中断是否会导致数据丢失”的迷思：

- **结论：** 现代MCU（NVIC控制器）拥有 **Pending（挂起）机制**。关中断期间触发的请求会被硬件标记。
    
- **现象：** 退出临界区（开中断）的瞬间，CPU会立即响应挂起的中断。
    
- **副作用：** 会增加中断延迟（Latency/Jitter），但不会丢失。
    
- **丢失的唯一条件：** 临界区耗时极长，长到同一种中断源触发了两次（Pending位被覆盖）。
    
- **最佳实践：** 临界区代码应极短，仅包含纯内存读写，禁包含复杂逻辑或延时。
    

## 3. 幽灵中断（Unhandled Interrupts）

当外设触发中断，但代码中未定义对应的 ISR（中断服务函数）时：

- **默认行为：** 启动文件（Startup code）通常利用 `Weak` 别名机制，将所有未定义中断指向 `Default_Handler`。
    
- **默认实现：** 通常是一个 `B .` (死循环)。
    
- **后果：** 系统表现为“突然卡死”，看门狗可能复位系统。
    
- **调试方法：** 查看 CPU 的 **IPSR 寄存器**，其值减去 16 即为当前触发的中断号（IRQ Number）。
    

---

# 工程级代码实现：智能 Default_Handler

在产品级代码中，`Default_Handler` 不应只是一个死循环。它应该能够：

1. **捕获现场：** 记录是哪个中断号导致了跑飞。
    
2. **便于调试：** 在开发模式下触发断点。
    
3. **自愈复位：** 在生产模式下记录错误后自动复位，防止设备“变砖”。
    

### 实现思路

利用 ARM CMSIS 接口读取 `IPSR` 寄存器，将其保存到不被复位清除的 RAM 区域（No-Init RAM）或全局变量中，以便复位后或连接调试器查看。

### 代码清单 (`stm32_debug_fault.c`)

C

```
#include "stm32f4xx_hal.h"  // 根据你的芯片型号替换，主要是为了 CMSIS 核心定义
#include <stdint.h>

/* ==========================================
 * 数据结构定义
 * ========================================== */

// 定义一个结构体用于保存崩溃信息
// 建议将此变量放置在 .noinit section，以便复位后还能读取（需修改链接脚本）
typedef struct {
    uint32_t magic_word;      // 用于判断数据是否有效
    uint32_t fault_irq_num;   // 触发异常的中断号 (IPSR)
    uint32_t r0;              // 简单的寄存器快照 (可选)
    uint32_t lr;              // 链接寄存器 (判断从哪里跳过来的)
} SystemCrashLog_t;

volatile SystemCrashLog_t g_crash_log;

#define CRASH_MAGIC 0xDEADBEEF

/* ==========================================
 * 核心实现：Default_Handler
 * ========================================== */

/**
 * @brief  重写 Default_Handler
 * @note   在 startup_stm32xxx.s 中，所有未定义的中断都弱定义到了这里。
 * 我们覆盖它，不再死循环，而是捕捉信息。
 */
void Default_Handler(void)
{
    /* 1. 获取当前正在执行的中断号 (IPSR 寄存器) */
    /* IPSR 低9位包含了 Exception Number */
    /* Exception Number = IRQn + 16 */
    uint32_t active_isr = __get_IPSR() & 0x1FF;

    /* 2. 记录崩溃现场 */
    g_crash_log.magic_word    = CRASH_MAGIC;
    g_crash_log.fault_irq_num = active_isr; // 关键信息！
    
    // 获取 LR 寄存器的大致值 (注意：在Handler模式下 LR 可能是 EXC_RETURN 值)
    register uint32_t lr_reg; 
    __asm volatile ("MOV %0, LR\n" : "=r" (lr_reg) );
    g_crash_log.lr = lr_reg;

    /* 3. 开发模式 vs 生产模式处理 */
    
#if defined(DEBUG) || defined(_DEBUG)
    /* --- 开发模式 --- */
    
    // 如果连接了调试器，直接触发断点，让开发者看到停在这里
    // 开发者此时查看 g_crash_log.fault_irq_num 即可知道是哪个外设没写IRQHandler
    // 例如：如果值为 16+6=22，查表可知是 EXTI0_IRQn
    __BKPT(0); 
    
    // 如果没有断点继续运行，进入死循环防止乱跑
    while(1) { }

#else
    /* --- 生产模式 --- */
    
    // 可以在这里执行 Flash 写入操作，将错误日志保存到 Flash
    // Log_SaveToFlash(&g_crash_log);
    
    // 必须复位系统，不能让设备挂死
    // 实际项目中建议延时几毫秒确保数据写完
    NVIC_SystemReset();
#endif
}

/* ==========================================
 * 辅助：HardFault 也能参考类似的思路
 * ========================================== */
// 注意：HardFault 处理通常需要汇编来提取栈帧（MSP或PSP），
// 下面是一个简化的 C 语言包装示例，实际入口通常是汇编。
void HardFault_Handler_C_Impl(uint32_t *stack_frame)
{
    // stack_frame[0] -> R0
    // stack_frame[1] -> R1
    // ...
    // stack_frame[6] -> PC (崩溃时的指令地址)
    
    volatile uint32_t crash_pc = stack_frame[6];
    
    #if defined(DEBUG)
        __BKPT(0);
    #else
        NVIC_SystemReset();
    #endif
}
```

### 如何使用这个代码进行 Debug？

假设你的设备运行中突然卡死（在Debug模式下）：

1. 调试器会停在 `__BKPT(0)` 这一行。
    
2. 在 Watch 窗口（或 Memory 窗口）查看全局变量 `g_crash_log`。
    
3. 读取 `g_crash_log.fault_irq_num` 的值。
    
4. **计算：** 假设读到的值是 **53**。
    
5. **查表：** `IRQ号 = 53 - 16 = 37`。
    
6. **对照：** 打开你的芯片数据手册（Datasheet）或 `stm32xxxx.h` 头文件，找到 IRQn_Type 枚举。
    
    - 查看第 37 号中断是谁？（假设是 `USART1_IRQn`）。
        
7. **结论：** 你开启了 USART1 的中断使能位，但你忘了实现 `void USART1_IRQHandler(void)` 函数。
    

### 总结

这段代码将“未知死机”变成了“已知错误”，通过捕获 `IPSR`，你可以直接定位到是哪个具体的硬件外设产生了“幽灵中断”。这是嵌入式系统从“玩具级”迈向“产品级”的重要一步。