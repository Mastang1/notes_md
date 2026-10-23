# 1. 
>1. dma是片上的外设资源，支持快速转存通过传输接口到临时接收buffer的dada到buffer；高级场景提供BD ring进行管理，方便后续流程的 0-copy应用。
>2. 总之就是可配的总线master，不占用cpu，帮助外设完成临时buffer数据的快速to ddr等功能；
>3. TODO：联系 DMA buffer整理dma硬件和该子系统的关联逻辑关系