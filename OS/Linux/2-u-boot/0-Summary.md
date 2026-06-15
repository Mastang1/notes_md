## 1.  uboot-kernel 过程pdf[[Embedded_Linux_Boot_Blueprint.pdf]]
 - 通过general register传递DTB指针；原因:不同设备ddr映射地址不一致，把DTB地址写固定，不具备通用性；
 - bootargs也是为kernel读取的一个信息，指定printf等
 - 在u-boot运行时，将bootcmd环境变量设置为tftp命令，实现镜像/dtb从PC读取，并执行boot；
 - 在kernel运行时（不确定，或者是u-boot），通过nft mount PC执行的文件路径到kernel的根路径，实现ko或者app的快速测试；
## 2. u-boot 核心执行序列
![[Pasted image 20260615222332.png]]

>  _**U-Boot核心要解决的问题总结：

>1. **突破SRAM尺寸壁垒**：通过SPL（Secondary Program Loader）完成早期时钟和DDR初始化，将执行空间从狭小的内部SRAM扩展到海量的外部DDR5。
>2. **重定位（Relocation）与空间保护**：Linux内核通常要求在DDR的低地址运行。U-Boot必须非常聪明地把自己的代码“搬家”到DDR的最高地址段（高地址），为内核腾出绝对纯净的底层空间，防止互相踩踏6。
>3. **复杂介质与文件系统抽象**：提供极其丰富的驱动（USB、TFTP、NFS、FAT、EXT4），使得内核镜像可以从网络、硬盘任意地方动态拉取，而不需要像MCU那样死板地烧录在固定Flash地址
---
## 3. 完整的基于tftp server及nfs实现快捷开发测试
![[Pasted image 20260615225216.png]]
