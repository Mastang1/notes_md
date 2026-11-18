## 1. 100M
>1. **ISR** 单纯最高优先级中断的响应耗时 （12 – 45 Cycles），0.12 - 0.45μs；
>2. **TASK** 在 ARM Cortex-M 下，通用 RTOS 上下文切换周期为 $T_{\text{关FPU}} \approx \mathbf{60 \sim 130\text{ cycles}}$，开启浮点后叠加寄存器保存开销则变为 $T_{\text{开FPU}} = T_{\text{关FPU}} + T_{\text{浮点}}(60 \sim 70) \approx \mathbf{120 \sim 200\text{ cycles}}$。
>3. 


