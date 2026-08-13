
## 总体思路模型
1. 定义诸如配置space结构等规范，实现RC初始化，枚举BDF并读取配置和业务的交互空间，提供ECMA功能；然后基于LDM提供基于VID：DID与driver match的规范；
	总体即：通过约定和RP递归读取EP信息实现枚举和配置获取，并提供了configuration space、business data space的访问机制，剩余任务就交给了EP driver，该driver
	 访问配置和数据域，加上dma，共同完成了业务功能。

## 1. 从LDM角度切入说明构成及总线实例化流程(总线拓扑参考[[01-硬件拓扑及理解]])
 - 1. Linux boot阶段实现vendor root complex driver的加载操作，该 driver属于platform bus type；该driver和dts由IP vendor开发；
 - 2. 在match后，调用rc driver的probe operation，实现rc初始化配置、PCIe域窗口配置，初始化bridge对象（包含vendor 私有data和
	 私有功能函数指针），作为参数传递到PCI HOST,实现vendor接口的封装；
 - 3. 通用接口`pci_host_probe(bridge)`调用。实现BDF枚举，BAR分配等操作；
 - 4. 后续操作，就进入到PCI BUS领域，从match-probe，实现module逻辑或者基于vfs提供user接口等业务逻辑；


## 2. 从硬件vendor IP原理切入、总结
 - 1. 功能提供：phy配置、RC配置，提供物理层建链接；通过递归遍历总线树，