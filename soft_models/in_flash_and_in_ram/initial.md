### 现代 MCU 代码运行介质总结 (200字)

在现代嵌入式 MCU 与 RTOS 架构中，代码（`.text`段）的运行介质主要分为**Flash就地执行（XIP）**与**RAM执行**两大阵营，其核心是容量、速度与功耗的博弈。

1. **Flash XIP（内/外部）**：当前最主流的方案。代码直接在片内或外挂 Flash 中运行，优势是容量大、成本低，不占用宝贵的 RAM；劣势是受限于总线速度，存在等待周期，高度依赖 Cache，且在擦写 Flash 时会阻塞中断响应。
    
2. **RAM 执行（SRAM/ITCM）**：追求极致的方案。启动时由汇编将代码从 Flash 搬运至 RAM。部分搬运至 **ITCM** 是为了获得零等待的硬实时响应（如电机 FOC 算法）；而**全 SRAM 运行**多用于可穿戴设备，通过彻底关闭 Flash 电源来实现极致续航。
    

现代工业级应用已抛弃绝对的“非此即彼”，普遍采用“海量业务跑 Flash + 关键算法/中断跑 RAM”的异构混合架构。

### 链接脚本 (LD Script) 核心片段对比

理解代码在哪运行，核心在于看懂链接脚本中的 **VMA（运行地址，`>` 符号后）** 和 **LMA（加载地址，`AT>` 符号后）**。

#### 1. 常规方案：.text 在 Flash 中就地执行 (XIP)

这是 90% 单片机工程的默认配置。代码被烧录在 Flash，CPU 直接去 Flash 取指令执行。

代码段

```c
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  /* 代码段和常量：运行在 Flash，加载在 Flash */
  .text :
  {
    . = ALIGN(4);
    *(.isr_vector) /* 中断向量表 */
    *(.text)       /* 常规代码 */
    *(.text*)
    *(.rodata)     /* 只读常量 */
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH      /* VMA: 运行在 FLASH */

  /* 已初始化变量：运行在 RAM，但初始值存在 Flash 中 */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT> FLASH /* VMA: RAM, LMA: FLASH */
}
```

#### 2. 高性能方案：.text 强制在 RAM 中运行

这种配置下，所有的代码虽然烧录在 Flash 中，但启动文件（Startup.s）必须把 `.text` 段从 Flash 拷贝到 RAM 中，然后 CPU 才开始执行。

代码段

```c
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  /* 仅保留启动所需的极少量代码在 Flash 中就地执行 */
  .startup :
  {
    . = ALIGN(4);
    *(.isr_vector)
    *startup_*.o(.text) /* 确保启动文件本身在 Flash 里，否则无人搬运 */
    . = ALIGN(4);
  } > FLASH

  /* 核心代码段：强制定向到 RAM 中运行 */
  _sflash_text = LOADADDR(.text); /* 获取在 Flash 中的起始存储地址 (LMA) */

  .text :
  {
    . = ALIGN(4);
    _sram_text = .;    /* 获取在 RAM 中的起始运行地址 (VMA) */
    
    *(.text)           /* 将所有的 C/C++ 业务代码放入此段 */
    *(.text*)
    *(.rodata)         /* 常量也一并放入 RAM，提高读取速度 */
    *(.rodata*)
    
    . = ALIGN(4);
    _eram_text = .;    /* 获取在 RAM 中的结束运行地址 */
  } > RAM AT> FLASH  /* 关键点：VMA 设置为 RAM，LMA 设置为 FLASH */
}
```

**关键差异提示：** 在第二个 RAM 执行的脚本中，`> RAM AT> FLASH` 这一句是灵魂。它告诉链接器分配 RAM 的内存空间供程序运行使用，但告诉烧录软件把这段机器码烧进 Flash 里。启动阶段需要用到 `_sflash_text`、`_sram_text` 和 `_eram_text` 这三个变量来实现内存大搬家。