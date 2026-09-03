|                                                                                |           |                                                                                         |
| ------------------------------------------------------------------------------ | --------- | --------------------------------------------------------------------------------------- |
| `libhailort`                                                                   | 4.23.0    | HailoRT 核心库                                                                             |
| `hailortcli`                                                                   | 4.23.0    | 命令行接口                                                                                   |
| `hailort-service`                                                              | 4.23.0    | 后台服务                                                                                    |
| `pyhailort`                                                                    | 4.23.0    | Python 接口                                                                               |
| `libgsthailo`                                                                  | 4.23.0    | gstreamer hailort 插件                                                                    |
| `hailo-pci`                                                                    | 4.23.0    | Hailo-8 PCIe 驱动                                                                         |
| `hailo-firmware`                                                               | 4.23.0    | 固件（已本地预置，md5 匹配）                                                                        |
| `packagegroup-hailo-hailort`                                                   | 1.0       | 集合包                                                                                     |
| `cxxopts` / `rapijson` / `rapidjson` / `xtensor` / `xtl` / `cppzmq` / `zeromq` | 各版本       | tappas 头文件/依赖                                                                           |
| `hailo-post-processes`                                                         | 5.1.0     | tappas 后处理                                                                              |
| `libgsthailotools`                                                             | 5.1.0     | tappas gstreamer 工具插件                                                                   |
| `packagegroup-hailo-tappas`                                                    | 1.0       | 集合包                                                                                     |
| `opencv`                                                                       | **4.4.0** | tappas 硬依赖（DL 已就位：opencv.git 3.3G / contrib 362M / 3rdparty 3.3G，4.4.0 各 SRCREV 均已校验存在） |

*为啥每次屏蔽这逼玩意*
BBMASK += "/work/sources/meta-alb/recipes-extended/opendds/"