# Reference Manual Index

本目录统一保存 Week 1 的原始资料。教程正文中的“章节号/小节号”优先使用已核实的正点原子官方在线文档或公开 PDF 版本。

> **页码规则**
>
> 1. `Explorer_STM32F4_V2.2_SCH.pdf` 是用户实际上传文件，因此本文给出精确 PDF 页码。
> 2. I.MX6ULL 的大型 PDF 当前并未作为本会话附件上传；公开仓库能够确认**版本、章节和小节**，但本环境无法可靠抽取它们的 PDF 物理页码。因此本文**不编造页码**，而是给出精确“第 X 章 / X.Y 小节 / 图号 / 在线直达链接”。
> 3. 如果以后把这些 PDF 按下表 ASCII 文件名放进本目录，可在 `source_index.md` 中补充 `#page=N` 深链接，而不会改变教程正文。

| Source ID | 手册 | 本地预期文件名 | 已核实定位 | 在线入口 |
|---|---|---|---|---|
| ALI-QUICK-1.8 | 正点原子 I.MX6U 用户快速体验 V1.8 | `ALIENTEK_iMX6U_Quick_Start_V1.8.pdf` | 第1章；2.1；2.2；3.11；第4章 | [PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%94%A8%E6%88%B7%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8CV1.8.pdf) / [Online](https://wiki.alientek.com/docs/category/imx6u-%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/) |
| ALI-DRV-1.5.2 | 正点原子 I.MX6U 嵌入式 Linux 驱动开发指南 V1.5.2 | `ALIENTEK_iMX6U_Linux_Driver_Guide_V1.5.2.pdf` | 第3章；第4章，重点 4.2、4.3 | [PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf) |
| ALI-CAPP-1.1 | 正点原子 I.MX6U 嵌入式 Linux C 应用编程指南 V1.1 | `ALIENTEK_iMX6U_Linux_C_Application_Guide_V1.1.pdf` | Week 1 仅辅助理解 Linux 用户态 | [PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%20C%E5%BA%94%E7%94%A8%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97V1.1.pdf) |
| ALI-NET-1.3.1 | 正点原子 I.MX6U 网络环境 TFTP&NFS 搭建手册 V1.3.1 | `ALIENTEK_iMX6U_TFTP_NFS_Guide_V1.3.1.pdf` | 网络拓扑、TFTP、NFS | [PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%BD%91%E7%BB%9C%E7%8E%AF%E5%A2%83TFTP%26NFS%E6%90%AD%E5%BB%BA%E6%89%8B%E5%86%8CV1.3.1.pdf) |
| F407-SCH-2.2 | 正点原子 Explorer STM32F4 V2.2 原理图 | `Explorer_STM32F4_V2.2_SCH.pdf` | **PDF p.1-p.5** | 本包已包含 |
| ZEPHYR-GS | Zephyr Getting Started | - | Ubuntu 24.04+ / west / SDK / build | [Official](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) |
| ZEPHYR-WEST | Zephyr west Basics | - | workspace / manifest / update | [Official](https://docs.zephyrproject.org/latest/develop/west/basics.html) |

## 本周最重要的手册定位

### 正点原子 I.MX6U 用户快速体验 V1.8
- **2.1.1**：CH340 驱动。
- **2.1.2**：串口终端；官方设置为 **115200 baud、flow control=None**。
- **2.2.3**：登录开发板；含启动介质拨码说明。
- **3.11**：网口测试；MINI 只支持一路 `eth0`，官方示例使用 `ifconfig` / `udhcpc -i eth0`。
- **第4章**：交叉编译。
- **4.6**：编译一个简单 C 文件。

### 正点原子 I.MX6U Linux 驱动开发指南 V1.5.2
- **第4章：开发环境搭建**。
- **4.2.1**：NFS 服务。
- **4.2.2**：SSH 服务。
- **4.3.1**：交叉编译器安装。
- **4.3.2**：旧版 Ubuntu 所需相关库。
- **4.3.3**：交叉编译器验证。
- 注意：该手册以较老 Ubuntu / Linaro 4.9.4 为开发基线；本课程的 Host 是 Ubuntu 24.04，所以 Week 1 的用户态练习优先使用 Ubuntu 24.04 自带的 `gcc-arm-linux-gnueabihf`，后续复现老 BSP 时再单独隔离 Linaro 4.9.4。

### Explorer STM32F4 V2.2 原理图
- **p.1**：LAN8720A Ethernet、WM8978。
- **p.2**：STM32F407ZET6、8 MHz HSE、32.768 kHz LSE、JTAG、USART1。
- **p.3**：W25Q128、SPI1、LED、KEY 等。
- **p.4**：CH340G USB-UART 和电源。
- **p.5**：PCB/连接器布局。
