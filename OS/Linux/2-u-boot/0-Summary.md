## 1.  uboot-kernel 过程pdf[[Embedded_Linux_Boot_Blueprint.pdf]]
 - 通过general register传递DTB指针；原因:不同设备ddr映射地址不一致，把DTB地址写固定，不具备通用性；
 - bootargs也是为kernel读取的一个信息，指定printf等
 - 在u-boot运行时，将bootcmd环境变量设置为tftp命令，实现镜像/dtb从PC读取，并执行boot；
 - 在kernel运行时（不确定，或者是u-boot），通过nft mount PC执行的文件路径到kernel的根路径，实现ko或者app的快速测试；