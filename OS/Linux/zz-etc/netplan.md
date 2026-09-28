Netplan 在设计上遵循了
_**前端声明与后端渲染解耦**_（Declarative Frontend & Backend Rendering）的架构理念。

  

==在系统启动（Boot）以及用户执行 `netplan apply` 时，Netplan 本质上是一个**代码生成器**（Generator），它将高层 YAML 文件翻译为底层的 `systemd-networkd` 或 `NetworkManager` 配置文件，最终由后端守护进程调用 Linux 内核 Netlink 接口完成网络配置。==

  

### 一、 系统启动阶段的自动解析与配置生成流程

在 Linux 系统启动早期（Early Boot Phase），`systemd` 会在网络服务启动之前触发 **System Generators**。Netplan 通过 `/lib/systemd/system-generators/netplan-generator` 挂载到该流程中：

代码段
![[Pasted image 20260928144801.png]]

### 二、 用户手动执行 `netplan apply` 的动态触发流程

当管理员在终端执行 `sudo netplan apply` 修改配置时，触发的是动态生成与后端重加载（Dynamic Reloading）流程：

  

代码段

![[Pasted image 20260928144728.png]]

### 三、 核心步骤的技术实现细节

1. **前端解析与语法树构建（Frontend & AST）**：
    
      
    - `libnetplan` 解析器读取配置文件时，遵循 `10-*.yaml` < `50-*.yaml` < `99-*.yaml` 的**字典序合并覆盖策略**。
        
          
        
    - 它将抽象的键值（如 `addresses: [192.168.1.100/24]`、`routes:`）转换为内部的 C 结构体描述网络节点。
        
          
        
2. **后端配置文件的生成目录（Transient Configuration）**：
    
      
    - Netplan **绝不会**修改原生的底层系统配置，所有自动生成的脚本均存储在**内存临时文件系统（tmpfs）** `/run` 目录下：
        
          
        - 如果 `renderer: networkd`：生成至 `/run/systemd/network/10-netplan-eth0.network` 等文件。
            
              
            
        - 如果 `renderer: NetworkManager`：生成至 `/run/NetworkManager/system-connections/netplan-eth0.nmconnection` 等 keyfile。
            
              
            
3. **后端执行与内核交互（Kernel Interface Activation）**：
    
      
    - 后端守护进程（`systemd-networkd`）读取 `/run/systemd/network/` 下的文件后，通过 **Netlink Socket (`AF_NETLINK`)** 协议直接与 Linux 内核通信：
        
          
        - `RTM_NEWLINK`：设置网络接口状态（Up/Down）、MTU、MAC 地址等；
            
              
            
        - `RTM_NEWADDR`：向内核绑订 IPv4/IPv6 地址；
            
              
            
        - `RTM_NEWROUTE`：向内核路由表（FIB）写入默认网关与静态路由。