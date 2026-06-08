## 1. Summary
 - 1. memory_map table是以 SOC中不同的CPU的视角来写的，核心CPU有 A53、CM7、LLCE、PFE、HSE、DMA(这货不能叫cpu，但是也访问memory)
 - 2. 每个CPU访问地址空间时候，会经过绑定的地址路由IP进行映射，最重的结果是：不同CPU访问相同地址，映射到了不同的地址空间；这么设计的原因：保障不同架构的CPU在实际用起来保持其原始架构，路由IP相当于一个硬件设计的适配层；
 - 3. 


## 2.  关键点—— QSPI AHB Buffer


## 3. 关键点 —— Internal SRAM(20M)


## 4. 关键点 —— 64bits(实际40bits)地址extended

