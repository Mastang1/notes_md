# Chapter 17 - Leave the MCU Physical-Memory Model: Virtual Address, Page and Mapping

> Week 3 / Day 3 - 从 MCU 物理地址直觉升级到 Linux VA/MMU 模型。

[← Part README](README.md) · [← Previous](ch16_elf_loader_dynamic_linker.md) · [Next →](ch18_file_descriptor_uapi.md)

## 17.1 为什么 Linux 程序看到的地址不能直接拿去查 DDR 地址

在 MCU 裸机/简单 RTOS 中，你看到 `0x20000000` 往往就是某片 SRAM 的物理地址。Linux 用户进程打印出的指针首先是**virtual address (VA)**。

费曼模型：每个进程拿到一套自己的“门牌号地图”；MMU/page table 决定这些门牌号实际通向哪一页物理内存。两个进程都可以有 `0x400000`，并不意味着它们访问同一物理位置。

## 17.2 从一个实验程序画出自己的地址空间

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int g_init = 1;
int g_bss;
static int s_data = 2;

int main(void)
{
    int stack = 3;
    void *heap = malloc(4096);
    printf("pid=%d\n", getpid());
    printf("main=%p g_init=%p g_bss=%p static=%p heap=%p stack=%p\n",
           main, &g_init, &g_bss, &s_data, heap, &stack);
    getchar();
}
```

另一个终端：

```bash
cat /proc/<pid>/maps
pmap -x <pid>
```

把打印地址落到 maps 的具体 VMA 区域。

## 17.3 `/proc/<pid>/maps` 不是 page table dump，而是 VMA 视图

你看到类似：

```text
address range  perms offset dev inode pathname
```

重点：

- `r-xp`：代码/可执行映像；
- `rw-p`：data/heap 等；
- `[heap]`；
- `[stack]`；
- shared libraries。

这是内核管理的 virtual memory area（VMA）层次，不是每一页 PTE 的逐项输出。

## 17.4 Page：为什么地址空间要切成固定粒度的页

典型 Linux page size 常见 4 KiB，但以：

```bash
getconf PAGESIZE
```

实测。

Page 带来：映射粒度、权限粒度、COW、demand paging 等能力。MMU translation 基本路径：

```text
VA
 -> TLB lookup
 -> miss: page table walk
 -> PA
 -> cache/memory
```

今天只建概念，后面 DMA/cache 会继续深化。

## 17.5 TLB：为什么 page table 不可能每次都从内存完整遍历

TLB 是地址翻译缓存。费曼类比：page table 是完整电话簿，TLB 是最近常用号码。TLB miss 才需要更昂贵的 page table walk。

不要把 TLB 和 data cache 混淆：一个缓存地址转换，一个缓存数据/指令。

## 17.6 `malloc()` 不是“系统调用一次就分配这么多物理内存”

用户 allocator 可能通过 `brk`/`mmap` 获取 address range，再自己切小块；Kernel 还可能利用 demand allocation，真正访问页面时才分配/建立 backing。

用：

```bash
strace -e brk,mmap,mprotect,munmap ./mem_demo
```

观察 libc allocator 和 syscall 的关系。

## 17.7 Guided Lab：fork COW 的地址实验

父子都打印 `&g_init`，virtual address 很可能相同。child 改写变量后，两者值不同。

你无法仅凭用户态指针证明 physical page 是否改变，但可以结合 COW 原理解释现象。进阶可在允许的系统上观察 `/proc/<pid>/pagemap`，但权限/接口限制不作为本章必做。

## 17.8 内核地址、用户地址、MMIO 地址：先分三类，不要混

```text
User VA       应用指针，受进程 page table 管理
Kernel VA     Kernel 自己的虚拟地址空间
Physical/MMIO 硬件总线/寄存器物理地址
```

后续 Driver 中 `ioremap()` 的意义就是：把 device physical/MMIO range 建立为 Kernel 可访问的 virtual mapping。

现在先记住问题，而不是提前学 API。

## 17.9 Independent Challenge：用 maps 解释一次 segmentation fault

尝试向字符串常量/只读映射写入（在受控测试程序中），观察 SIGSEGV，结合 maps 的权限解释为什么这不是“C 语言随机崩了”。

## 17.10 下一章：进程有自己的地址空间，但它怎样引用 Kernel 中的“打开对象”？

Chapter 18 进入 file descriptor。你将看到 `int fd` 如何通向 `struct file`，再通向 `file_operations`，这正是未来字符驱动 UAPI 的入口。

## References and manuals

### ALIENTEK Linux C Application Guide V1.1
- Local expected path: `../references/ALIENTEK_iMX6ULL_Linux_C_Application_Guide_V1.1.pdf`
- Online: [ALIENTEK Linux C Application Guide V1.1](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf)
- 本章阅读定位：查进程内存、malloc/mmap、/proc 相关章节。

### proc_pid_maps(5)
- Online: [proc_pid_maps(5)](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html)
- 本章阅读定位：读 maps 字段与权限语义。

### mmap(2)
- Online: [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html)
- 本章阅读定位：先读用户态映射语义，为后续 Driver mmap 铺垫。

- [Unified source index](../common/source_index.md)

[← Part README](README.md) · [← Previous](ch16_elf_loader_dynamic_linker.md) · [Next →](ch18_file_descriptor_uapi.md)
