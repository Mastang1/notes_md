## 1. cortexM 启动过程
>依赖机制说明：CPU 被设计为从地址4取指令执行(reset_handler 地址)，0 地址放的是 栈顶指针（地址）；然后，CPU执行指令，并执行栈操作；
>也就是，上来执行的是定义的 reset_handler 函数

>所以，代码开发中要想办法把reset_handler放到该地址；
>编译器编译后代码分为 text/data/BSS等段，Startup代码中把 isr_vector 作为中断向量表的段；
>由Linker读取link.ld文件实现各个段的排序。比如从片上flash启动（0x08000000）时候，link文件将向量表存储在flash起始位置。

>但是data需要存储在flash，同时运行时候在ram，所以采用_>RAM AT > FLASH_方式：data段的符号地址从0x20000000开始，但是实际存储在flash；
>BSS段：未初始化的全局或者局部静态变量，只用占位，不必占用flash空间；

>最后，因为data段的加载地址和实际运行地址不同，需要执行数据从flash拷贝到ram操作，一般在reset_hander中实现；同时，完成BSS段的clear操作；

>然后，跳转到具体应用entry

```c
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    .text : {
        KEEP(*(.isr_vector))    /* defined in startup code */
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH   /* LMA = VMA , FLASH is executable, VMA=0x08000000 */

    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH   /* VMA=0x20000000, LMA= follows FLASH */

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM /*IN-RAM, which is zero-initialized, so no need to copy from LMA*/

    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
```



## 2. bootloader 跳转APP过程

## 3. RTOS任务切换过程