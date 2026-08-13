## 1. 从LDM角度切入说明构成及总线实例化流程
 - 1. Linux boot阶段实现vendor root complex driver的加载操作，该 driver属于platform bus type；该driver和dts由IP vendor开发；
 - 2. 在match后，调用rc driver的probe operation，实现rc初始化配置、PCIe域窗口配置，初始化bridge对象（包含vendor 私有data和
	 私有功能函数指针），作为参数传递到PCI HOST,实现vendor接口的封装；
 - 3. 通用接口`pci_host_probe(bridge)`调用。实现


## 2. 从硬件vendor IP原理切入、总结
 - 1. 