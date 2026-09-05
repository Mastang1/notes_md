# 2026–2027 高级异构 SoC / Linux BSP & Driver / RTOS Platform 20 周实战计划

**版本：2026-09 重构版**  
**时间预算：每天约 2 小时；20 周；Linux/SoC 主线约 75%，Zephyr/Bootloader 产品线约 25%。**

> 目标不是“把 Linux 和 Zephyr 都学一遍”，而是形成一条可面试、可落地、可证明的系统软件能力链：`Boot → BSP → Driver → DMA/Cache → PCIe/IPC → Runtime/Yocto`，同时用 `Zephyr + MCUboot` 补齐 RTOS 平台生态和产品级固件更新。


## 0. 个人能力拼图与本计划补齐目标

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 860 570" role="img" aria-label="个人能力拼图" style="font-family:'Noto Sans CJK SC','Microsoft YaHei',sans-serif"><defs><filter id="shadow" x="-20%" y="-20%" width="140%" height="140%"><feDropShadow dx="0" dy="4" stdDeviation="5" flood-color="#000" flood-opacity="0.16"/></filter></defs><rect width="860" height="570" rx="24" fill="#F7F9FC"/><text x="40" y="47" font-size="28" font-weight="700" fill="#1F2D3D">个人能力拼图 - 目标：高级异构 SoC / System Software</text><text x="40" y="72" font-size="14" fill="#657386">颜色表示当前状态，不表示重要性；红色区域即本 20 周计划的主要补齐对象。</text><path d="M 40.0 95.0 L 300.0 95.0 L 300.0 141.8 C 300.0 147.8, 314.0 145.0, 314.0 155.0 C 314.0 165.0, 300.0 162.2, 300.0 168.2 L 300.0 215.0 L 187.0 215.0 C 181.0 215.0, 180.0 201.0, 170.0 201.0 C 160.0 201.0, 159.0 215.0, 153.0 215.0 L 40.0 215.0 L 40.0 95.0 Z" fill="#2E8B57" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="170.0" y="143" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">嵌入式 C / ARM</text><text x="170.0" y="170" text-anchor="middle" font-size="13" fill="#F7FBFF">startup · linker · NVIC</text><path d="M 300.0 95.0 L 560.0 95.0 L 560.0 141.8 C 560.0 147.8, 546.0 145.0, 546.0 155.0 C 546.0 165.0, 560.0 162.2, 560.0 168.2 L 560.0 215.0 L 447.0 215.0 C 441.0 215.0, 440.0 229.0, 430.0 229.0 C 420.0 229.0, 419.0 215.0, 413.0 215.0 L 300.0 215.0 L 300.0 168.2 C 300.0 162.2, 314.0 165.0, 314.0 155.0 C 314.0 145.0, 300.0 147.8, 300.0 141.8 L 300.0 95.0 Z" fill="#2E8B57" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="430.0" y="143" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">MCU / RTOS Kernel</text><text x="430.0" y="170" text-anchor="middle" font-size="13" fill="#F7FBFF">FreeRTOS · VxWorks · 调度</text><path d="M 560.0 95.0 L 820.0 95.0 L 820.0 215.0 L 707.0 215.0 C 701.0 215.0, 700.0 201.0, 690.0 201.0 C 680.0 201.0, 679.0 215.0, 673.0 215.0 L 560.0 215.0 L 560.0 168.2 C 560.0 162.2, 546.0 165.0, 546.0 155.0 C 546.0 145.0, 560.0 147.8, 560.0 141.8 L 560.0 95.0 Z" fill="#2E8B57" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="690.0" y="143" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">硬件与调试</text><text x="690.0" y="170" text-anchor="middle" font-size="13" fill="#F7FBFF">示波器 · JTAG · Trace32</text><path d="M 40.0 215.0 L 153.0 215.0 C 159.0 215.0, 160.0 201.0, 170.0 201.0 C 180.0 201.0, 181.0 215.0, 187.0 215.0 L 300.0 215.0 L 300.0 261.8 C 300.0 267.8, 286.0 265.0, 286.0 275.0 C 286.0 285.0, 300.0 282.2, 300.0 288.2 L 300.0 335.0 L 187.0 335.0 C 181.0 335.0, 180.0 349.0, 170.0 349.0 C 160.0 349.0, 159.0 335.0, 153.0 335.0 L 40.0 335.0 L 40.0 215.0 Z" fill="#2E8B57" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="170.0" y="263" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">协议与工程实现</text><text x="170.0" y="290" text-anchor="middle" font-size="13" fill="#F7FBFF">CAN/LIN · TCP/IP · 滑窗重传</text><path d="M 300.0 215.0 L 413.0 215.0 C 419.0 215.0, 420.0 229.0, 430.0 229.0 C 440.0 229.0, 441.0 215.0, 447.0 215.0 L 560.0 215.0 L 560.0 261.8 C 560.0 267.8, 574.0 265.0, 574.0 275.0 C 574.0 285.0, 560.0 282.2, 560.0 288.2 L 560.0 335.0 L 447.0 335.0 C 441.0 335.0, 440.0 321.0, 430.0 321.0 C 420.0 321.0, 419.0 335.0, 413.0 335.0 L 300.0 335.0 L 300.0 288.2 C 300.0 282.2, 286.0 285.0, 286.0 275.0 C 286.0 265.0, 300.0 267.8, 300.0 261.8 L 300.0 215.0 Z" fill="#3778C2" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="430.0" y="263" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">异构 SoC / IPC</text><text x="430.0" y="290" text-anchor="middle" font-size="13" fill="#F7FBFF">A53↔MCU · SHM · IPCF</text><path d="M 560.0 215.0 L 673.0 215.0 C 679.0 215.0, 680.0 201.0, 690.0 201.0 C 700.0 201.0, 701.0 215.0, 707.0 215.0 L 820.0 215.0 L 820.0 335.0 L 707.0 335.0 C 701.0 335.0, 700.0 349.0, 690.0 349.0 C 680.0 349.0, 679.0 335.0, 673.0 335.0 L 560.0 335.0 L 560.0 288.2 C 560.0 282.2, 574.0 285.0, 574.0 275.0 C 574.0 265.0, 560.0 267.8, 560.0 261.8 L 560.0 215.0 Z" fill="#F0A43A" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="690.0" y="263" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">Yocto / Linux 应用</text><text x="690.0" y="290" text-anchor="middle" font-size="13" fill="#F7FBFF">构建 · 集成 · 依赖 · 用户态</text><path d="M 40.0 335.0 L 153.0 335.0 C 159.0 335.0, 160.0 349.0, 170.0 349.0 C 180.0 349.0, 181.0 335.0, 187.0 335.0 L 300.0 335.0 L 300.0 381.8 C 300.0 387.8, 314.0 385.0, 314.0 395.0 C 314.0 405.0, 300.0 402.2, 300.0 408.2 L 300.0 455.0 L 40.0 455.0 L 40.0 335.0 Z" fill="#D85B5B" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="170.0" y="383" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">Linux BSP / Driver</text><text x="170.0" y="410" text-anchor="middle" font-size="13" fill="#F7FBFF">DTS · probe · IRQ · VFS</text><path d="M 300.0 335.0 L 413.0 335.0 C 419.0 335.0, 420.0 321.0, 430.0 321.0 C 440.0 321.0, 441.0 335.0, 447.0 335.0 L 560.0 335.0 L 560.0 381.8 C 560.0 387.8, 546.0 385.0, 546.0 395.0 C 546.0 405.0, 560.0 402.2, 560.0 408.2 L 560.0 455.0 L 300.0 455.0 L 300.0 408.2 C 300.0 402.2, 314.0 405.0, 314.0 395.0 C 314.0 385.0, 300.0 387.8, 300.0 381.8 L 300.0 335.0 Z" fill="#D85B5B" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="430.0" y="383" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">内存 / DMA / PCIe</text><text x="430.0" y="410" text-anchor="middle" font-size="13" fill="#F7FBFF">cache · barrier · IOMMU · BAR</text><path d="M 560.0 335.0 L 673.0 335.0 C 679.0 335.0, 680.0 349.0, 690.0 349.0 C 700.0 349.0, 701.0 335.0, 707.0 335.0 L 820.0 335.0 L 820.0 455.0 L 560.0 455.0 L 560.0 408.2 C 560.0 402.2, 546.0 405.0, 546.0 395.0 C 546.0 385.0, 560.0 387.8, 560.0 381.8 L 560.0 335.0 Z" fill="#D85B5B" stroke="#FFFFFF" stroke-width="3" filter="url(#shadow)"/><text x="690.0" y="383" text-anchor="middle" font-size="18" font-weight="700" fill="#FFFFFF">RTOS 平台生态</text><text x="690.0" y="410" text-anchor="middle" font-size="13" fill="#F7FBFF">Zephyr · MCUboot · DFU</text><rect x="45" y="490" width="18" height="18" rx="4" fill="#2E8B57"/><text x="72" y="504" font-size="14" fill="#34495E">已形成优势</text><rect x="235" y="490" width="18" height="18" rx="4" fill="#3778C2"/><text x="262" y="504" font-size="14" fill="#34495E">已有项目/可继续放大</text><rect x="425" y="490" width="18" height="18" rx="4" fill="#F0A43A"/><text x="452" y="504" font-size="14" fill="#34495E">已有基础但不成体系</text><rect x="615" y="490" width="18" height="18" rx="4" fill="#D85B5B"/><text x="642" y="504" font-size="14" fill="#34495E">当前优先缺口</text><rect x="40" y="530" width="780" height="1" fill="#D9E1EA"/><text x="40" y="553" font-size="13" fill="#526273">本计划的核心动作：把 Linux Driver / DMA / PCIe 与 RTOS 平台生态补齐，并与现有 IPCF、Yocto、Hailo8 项目形成一条连续的系统软件能力链。</text></svg>

### 0.1 结论

- **保留并放大**：Cortex-M/ARM、C、MCAL、FreeRTOS/VxWorks、硬件调试、通信协议、异构 IPC。
- **重点补齐**：Linux Driver/BSP、MMU/DMA/cache/PCIe、大厂式 Linux 调试、RTOS 平台生态。
- **新增产品型项目**：STM32F407 + Zephyr + MCUboot + MCUmgr Secure DFU。暂不投入 AI Runtime。

## 1. 两块开发板的职责划分

### 1.1 正点原子 i.MX6ULL：Linux 主训练平台

用于 U-Boot、Kernel、Device Tree、platform driver、VFS、GPIO/IRQ、mmap、DMA 认知、ftrace/perf/kgdb 等。它的价值是让你把 Linux BSP/Driver 从“源码理解”变成“真实板卡闭环”。

### 1.2 正点原子 STM32F4“外星人/探索者”板：Zephyr + Bootloader 产品平台

本计划**按常见探索者 STM32F4 / STM32F407ZGT6**设计。该 MCU 与 Zephyr 已维护的 STM32F4 Discovery 同属 STM32F407xx；正点原子板本身不是 Zephyr 上游现成 board target，所以我们自己做一次 out-of-tree board port。这个工作量可控，而且正好训练 DTS/Kconfig/device model。若你手上板卡实际是 F429/F407 的其他版本，第 1 周先按芯片丝印和原理图调整 SoC/clock/pin 配置，计划不变。
常见探索者 F407 资源可支持本项目：1 MB 片内 Flash、192 KB RAM，以及板载 W25Q128 SPI Flash；先用片内 Flash 跑通 MCUboot，外部 Flash 作为后续大镜像 staging 的选做项。

## 2. 开发环境：一次搭好，20 周不再折腾

### 2.1 总体结构

```text
Windows 11 Host
└─ VMware Workstation
   └─ Ubuntu 24.04 LTS（主开发机）
      ├─ Linux/6ULL: ARM Linux cross-toolchain + TFTP + NFS + gdb
      ├─ Zephyr: Python venv + west + Zephyr SDK(arm-zephyr-eabi)
      ├─ QEMU: Oops/KASAN/kgdb 等可破坏实验
      └─ Docker/容器（仅当正点原子旧 BSP 在新宿主机编译不兼容时使用）

硬件
├─ i.MX6ULL  ← Ethernet + UART
└─ STM32F407 ← SWD/JTAG + UART
```
**为什么主机选 Ubuntu 24.04：**当前 Zephyr Getting Started 以 Ubuntu 24.04 LTS 及更新版本为直接支持基线；老 6ULL BSP 若出现 host-tool 兼容问题，用容器隔离，不让旧 BSP 绑架整个开发环境。

### 2.2 Linux/6ULL 主机准备清单

```bash
sudo apt update
sudo apt install -y git build-essential make cmake ninja-build gdb-multiarch \
  device-tree-compiler u-boot-tools tftpd-hpa nfs-kernel-server \
  openssh-server rsync ccache pkg-config flex bison bc libssl-dev \
  libncurses-dev python3 python3-venv python3-pip strace trace-cmd
```
目录统一：`~/work/linux/`、`~/work/zephyr/`、`~/work/tools/`、`~/nfs/`、`/srv/tftp/`。源码、工具链、输出不要混在一个目录。

### 2.3 Zephyr 环境准备清单

按当前官方推荐，用独立 Python virtual environment，避免系统 Python 被 west/Zephyr requirements 污染。
```bash
python3 -m venv ~/work/zephyr/.venv
source ~/work/zephyr/.venv/bin/activate
pip install west
west init -m https://github.com/zephyrproject-rtos/zephyr ~/work/zephyr/ws
cd ~/work/zephyr/ws
west update
west packages pip --install
west zephyr-export
cd zephyr
west sdk install --toolchains arm-zephyr-eabi
```
首次只验证官方 target：`west build -p always -b stm32f4_disco samples/hello_world`。**不要第一天就开始改 正点原子 board**；先确认 host、SDK、west 都正常，再进入 board port。

### 2.4 F407 Zephyr Board Port 最小目录

```text
my-zephyr-platform/
├─ boards/alientek/f407_explorer/
│  ├─ board.yml
│  ├─ board.cmake
│  ├─ Kconfig.f407_explorer
│  ├─ Kconfig.defconfig
│  ├─ f407_explorer_defconfig
│  └─ f407_explorer.dts
├─ app/
├─ platform/
│  ├─ health/
│  ├─ shell/
│  └─ update/
└─ tools/
   └─ fw_update.py
```
Board port 只先做四件事：**SoC、clock、console、LED**。UART/LED 成功后再逐步加 KEY、SPI Flash、MCUboot partitions。

## 3. RTOS 产品实战：Zephyr + MCUboot + MCUmgr Secure DFU

### 3.1 为什么选 MCUboot，而不是自己写教学 Bootloader

MCUboot 是跨 RTOS 的安全 Bootloader，Zephyr 的 DFU 与 MCUmgr 已直接集成。它能训练真正产品会遇到的：镜像签名、版本、slot、test/confirm、失败回滚、升级状态、Host 升级工具，而不是只训练“跳转到 App 地址”。

### 3.2 目标架构

```text
PC Host Updater (Python / mcumgr)
          │ UART/SMP（第一阶段）
          ▼
+-----------------------------+
| Zephyr Application          |
|  Shell / Health / Version   |
|  MCUmgr SMP Server          |
+-------------+---------------+
              │ image management
              ▼
+-----------------------------+
| MCUboot                     |
| signature verify            |
| test / confirm / rollback   |
+-------------+---------------+
              │
              ▼
        STM32F407 Flash
```
第一版只要求 UART DFU，因为它最容易在 2–3 周内做成可复用产品功能；“OTA”只是传输层变化。以后如果板载 Ethernet 路径稳定，可把 MCUmgr transport 扩到 UDP，但 Secure Boot/slot/rollback 主体不变。

### 3.3 F407 学习版 Flash Layout（需以实际镜像尺寸复核）

| 区域 | 地址范围 | 大小 | 目的 |
|---|---|---:|---|
| MCUboot | `0x08000000–0x0801FFFF` | 128 KB | sectors 0–4，Bootloader |
| slot0 | `0x08020000–0x0807FFFF` | 384 KB | sectors 5–7，当前应用 |
| slot1 | `0x08080000–0x080FFFFF` | 512 KB | sectors 8–11，升级候选 + swap extra sector |
这个布局利用 F407 后半区统一 128 KB sector，适合研究 MCUboot `swap-using-offset`。但 MCUboot trailer/sector 几何会压缩应用有效容量；如果你的 Zephyr App 超出预算，**不要硬挤功能**，后续把 staging 移到板载 W25Q128 或改升级策略。产品最终布局必须以实际 image size、flash erase geometry 和掉电测试结果重新评审。

### 3.4 RTOS 项目最终验收

- 正点原子 F407 自定义 Zephyr board target，可 clean build/flash/debug。
- Zephyr App 有 logging、shell、version、health/watchdog 基础服务。
- MCUboot 使用自己生成的开发签名 key，不使用公开 demo private key。
- PC 通过 UART 上传 signed image；支持 image list/test/reset/confirm。
- 未 confirm 的新镜像能够回滚；坏签名/损坏镜像被拒绝。
- 有 Python updater、升级 SOP、failure-injection test report。

## 4. 每日执行规则

每个 2 小时学习块统一按：**15 min 回忆/画图 → 35 min 定向学习 → 60 min 实验 → 10 min Git/记录**。遇到问题先按“日志→源码→官方文档→AI code review”的顺序，不允许一上来让 AI 生成完整 Driver。
建议 Git 仓库：
```text
soc-system-20w/
├─ linux/
│  ├─ env/  kernel/  drivers/  debug/  boot/  pcie/  ipc/
├─ zephyr-f407/
│  ├─ boards/  app/  platform/  mcuboot/  tools/  tests/
├─ notes/
└─ interview/
```

## 5. 20 周详细日计划

### Week 1: 开发主机与双平台环境搭建

**周验收：Ubuntu 开发主机、6ULL 基础链路、Zephyr workspace 三套环境均可复现。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **搭建主开发机** | VMware 新建 Ubuntu 24.04 LTS；CPU≥4核、RAM≥8GB、磁盘≥120GB；网卡设桥接；创建快照 `clean-os`。建立 `~/work/linux`、`~/work/zephyr`、`~/work/tools`、`~/nfs`、`/srv/tftp`。 | Ubuntu 可联网；`ip a` 能看到桥接地址；目录与快照完成。 |
| Day 2 | Linux 2h | **建立编译工具基线** | 安装 git/make/cmake/ninja/gdb/strace/readelf/objdump 等；安装 ARM Linux 交叉工具链；分别编译 x86 与 ARM `hello.c`，用 `file/readelf -h` 比较 ELF。 | 能解释 Host GCC 与 Target GCC；保存 `toolchain_baseline.md`。 |
| Day 3 | Linux 2h | **6ULL 串口与网络** | 连接 USB-TTL；记录 U-Boot/Linux 串口参数；给 Ubuntu 和 6ULL 设置同网段静态 IP；验证 `ping`、SSH、SCP。 | PC↔6ULL 双向 ping；SSH 登录；保存网络拓扑与 IP。 |
| Day 4 | Linux 2h | **TFTP/NFS 开发链路** | 安装并配置 TFTP、NFS；U-Boot 下 `ping serverip`、`tftp` 下载一个测试文件；Linux 下挂载 `~/nfs`。 | 6ULL 能从 Ubuntu TFTP/NFS 取文件；记录服务配置。 |
| Day 5 | Zephyr 2h | **Zephyr 官方环境** | 创建 Python venv；安装 west；`west init/update`；安装 Zephyr SDK ARM toolchain；先对官方 `stm32f4_disco` 构建 `hello_world`。 | `west build` 成功，理解 west workspace/manifest/module。 |
| Day 6 | Zephyr 2h | **确认 F4 硬件与调试探针** | 确认开发板丝印/主芯片/晶振/UART/LED/KEY/SWD/JTAG；若为探索者 F4，按 STM32F407ZGT6 记录 1MB Flash、192KB RAM、W25Q128 等资源。确认手头 J-Link/ST-Link；画最小板级资源表。 | 形成 `f407_board_audit.md`；若 MCU 不是 F407ZGT6，在此处更新后续 DTS 参数。 |
| Day 7 | 复盘 1-2h | **环境复现检查** | 关闭所有终端，从零重新激活 Linux 与 Zephyr 环境；执行工具版本检查；把关键配置加入 Git；为 Ubuntu 再做 `env-ready` 快照。 | 不用搜索聊天记录即可恢复三个环境。 |


### Week 2: 6ULL 构建闭环 + 正点原子 F407 Zephyr Board Port

**周验收：6ULL 能独立编译/替换 U-Boot、Kernel、DTB；F407 自定义 Zephyr board 能进入编译链。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **认识 6ULL BSP 目录** | 定位 U-Boot、Kernel、DTB、rootfs、交叉工具链；写出源码→产物→板卡位置映射。不要先看教程结论，先用 `find/file` 自己找。 | 输出 `6ull_artifact_map.md`。 |
| Day 2 | Linux 2h | **第一次完整编译** | 独立编译 U-Boot、Kernel、DTB；记录 defconfig、ARCH、CROSS_COMPILE、产物路径和构建耗时。 | 三个产物生成；能说明 `zImage/vmlinux/dtb` 区别。 |
| Day 3 | Linux 2h | **网络替换 Kernel/DTB** | U-Boot 用 TFTP 加载 zImage 与 DTB；不烧写 eMMC/SD，先 RAM 启动；修改 DTS 中一个无风险属性验证 DTB 真被替换。 | 能快速完成“改 DTS→编译→TFTP→重启验证”。 |
| Day 4 | Linux 2h | **GDB 用户态远程调试** | 交叉编译带 `-g` 的 ARM 程序；板端 `gdbserver`，Host `gdb-multiarch`；完成断点、寄存器、栈、反汇编。 | 保存 `gdb_remote_session.txt`。 |
| Day 5 | Zephyr 2h | **创建 out-of-tree board** | 按 Zephyr hardware model v2 创建 `boards/alientek/f407_explorer/`：`board.yml`、DTS、Kconfig、defconfig、board.cmake。先继承 STM32F407 SoC，不加外设。 | `west boards` 能看到自定义 board；hello_world 能编译。 |
| Day 6 | Zephyr 2h | **时钟、console 与下载** | 从 F407 原理图确认 HSE、console UART、pinctrl；配置 `chosen { zephyr,console = ... }`；配置 J-Link/OpenOCD runner；首次上板打印。 | 串口打印 Zephyr banner/hello；能 `west flash` 或明确记录替代下载命令。 |
| Day 7 | 复盘 1-2h | **Clean build 复现** | 删除 build 目录，从 Git checkout 后重新编译 6ULL DTB 和 Zephyr hello_world；总结“Linux DTS 与 Zephyr DTS 同与不同”。 | 两套 clean build 均成功。 |


### Week 3: Linux 用户态系统基础 + Zephyr 基础外设

**周验收：能从 syscall/ELF/FD 解释用户程序；Zephyr 自定义板完成 LED/KEY 与调试。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **进程与 syscall** | 写 fork/exec/wait 小程序；用 `strace -f -tt -T` 跟踪；画 Application→libc→syscall→kernel。 | 能说明进程创建与程序加载不是一件事。 |
| Day 2 | Linux 2h | **ELF 与链接** | 用 `readelf -h/-S/-l/-s`、`objdump -d`、`nm` 分析程序；对比 section 与 segment；定位 entry。 | 从 ELF 回答 text/data/bss/relocation 基础问题。 |
| Day 3 | Linux 2h | **虚拟地址空间** | 打印 text/global/heap/stack 地址；查看 `/proc/<pid>/maps`、`pmap`；解释 MCU 物理内存模型与 Linux VA 模型差别。 | 画进程 VA 图。 |
| Day 4 | Linux 2h | **FD 与设备接口预习** | 写 `open/read/write/poll/ioctl` 用户程序骨架；用 strace 看 syscall 参数；理解 fd→struct file→file_operations。 | 形成后续驱动测试工具 `user_tool.c`。 |
| Day 5 | Zephyr 2h | **LED/KEY Devicetree** | 给自定义 board 加 gpio-leds、gpio-keys/aliases；运行 blinky/button 示例；观察 generated devicetree。 | LED/KEY 都工作；提交 DTS。 |
| Day 6 | Zephyr 2h | **west debug** | 使用 `west debug` 或 J-Link GDB server；断点到 main/thread；观察线程栈与寄存器。 | 保存一次完整 debug 记录。 |
| Day 7 | 复盘 1-2h | **费曼复述** | 不看资料讲清 ELF、syscall、FD、Zephyr DTS；把不确定点列成 5 个问题，下周逐个解决。 | 录一份 10 分钟口述提纲。 |


### Week 4: Kernel/Kbuild/Module + Zephyr Kernel 使用

**周验收：能自己写/编译/加载内核模块；Zephyr 任务与同步不再停留在 API 调用层。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **Kernel 构建体系** | 理解 Kconfig→.config→Kbuild→vmlinux→zImage→modules→dtbs；用 `make menuconfig` 改一个配置并确认 `.config` 差异。 | 输出构建流程图。 |
| Day 2 | Linux 2h | **Hello module** | 写 out-of-tree `.ko`；实现 init/exit/module_param；掌握 insmod/rmmod/modinfo/dmesg。 | 模块可重复加载卸载 20 次。 |
| Day 3 | Linux 2h | **symbol/vermagic** | 查看 `/proc/kallsyms`、`nm *.ko`；理解 EXPORT_SYMBOL、模块依赖、vermagic；故意用不匹配模块观察报错。 | 能定位 `invalid module format`。 |
| Day 4 | Linux 2h | **最小 Oops 认知** | QEMU 或可恢复环境中制造 NULL dereference；保存 dmesg；用 addr2line/objdump 映射源码位置。 | 完成 `oops_basic.md`。 |
| Day 5 | Zephyr 2h | **线程/调度/同步** | 实现 3 个线程，不同优先级；使用 semaphore/queue/timer；对照 FreeRTOS 的 task/list/queue 心智模型。 | 用日志证明调度与唤醒顺序。 |
| Day 6 | Zephyr 2h | **Stack/Heap/Thread Analyzer** | 开启 thread analyzer、stack sentinel/保护；故意把线程栈配置过小并观察诊断；记录系统 RAM 使用。 | 能解释 RTOS 产品为何必须做 stack budget。 |
| Day 7 | 复盘 1-2h | **两套 OS 对照** | 做一张 Linux process/thread 与 Zephyr thread/context 对照表；只保留真正影响开发的差异。 | 提交 `linux_vs_zephyr_runtime.md`。 |


### Week 5: Linux Device Tree / Driver Model + Zephyr Devicetree/Kconfig

**周验收：真正打通 Linux DTS→match→probe；理解 Zephyr DTS 与 Kconfig 的职责分离。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **DTS 解析** | 用 `dtc` 反编译 DTB；追踪 include；找 6ULL 一个 UART/GPIO 节点；识别 reg/interrupts/clocks/pinctrl/status。 | 能够从 DTB 找到真实硬件资源。 |
| Day 2 | Linux 2h | **platform_driver** | 写 `of_match_table + platform_driver`；让 DTS compatible 匹配并进入 probe/remove。 | dmesg 显示 probe；禁用节点后不 probe。 |
| Day 3 | Linux 2h | **resource API** | 在 probe 中使用 `platform_get_resource/devm_ioremap_resource/platform_get_irq`；先只打印资源，不控制硬件。 | 能说明资源是谁创建、谁消费。 |
| Day 4 | Linux 2h | **/sys 观察 Driver Model** | 查看 `/sys/bus/platform/{devices,drivers}`、device symlink、uevent；手工 bind/unbind 一个安全驱动或自写驱动。 | 把 LDM 理论落到实际 sysfs 对象。 |
| Day 5 | Zephyr 2h | **DTS binding/overlay** | 阅读一个 GPIO/SPI binding；写 overlay 改节点；查看 `zephyr.dts`、generated headers；理解 compatible→driver instance。 | 能从 DTS 属性定位到宏展开结果。 |
| Day 6 | Zephyr 2h | **Kconfig/menuconfig** | 创建自定义 `CONFIG_APP_*`；用 prj.conf/menuconfig 控制功能；理解 DTS=硬件事实、Kconfig=软件选择。 | 构建两个 feature variant。 |
| Day 7 | 复盘 1-2h | **画双系统资源模型** | 画 Linux platform driver 与 Zephyr device model 对照图。 | 面试可口述 5 分钟。 |


### Week 6: VFS/字符设备 + Zephyr 平台服务骨架

**周验收：Linux 用户态到 Driver 调用闭环；Zephyr 项目开始具备日志、Shell、健康管理。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **miscdevice** | 实现 miscdevice + file_operations：open/release/read/write；用户程序完成基本访问。 | `/dev/mydev` 可稳定读写。 |
| Day 2 | Linux 2h | **user/kernel copy** | 加入 copy_to_user/copy_from_user；故意传非法指针观察 EFAULT；理解用户指针不能直接解引用。 | 错误路径有明确 errno。 |
| Day 3 | Linux 2h | **ioctl ABI** | 设计 versioned ioctl：GET_INFO/SET_MODE/RESET；使用 `_IOR/_IOW/_IOWR`；考虑 32/64 位结构体对齐。 | 形成 `uapi/mydev.h`。 |
| Day 4 | Linux 2h | **sysfs/debugfs** | 给设备增加只读状态和 debugfs 统计；比较 `/dev`、sysfs、debugfs 各自适用场景。 | 接口职责清楚，不把控制接口乱塞 sysfs。 |
| Day 5 | Zephyr 2h | **Logging + Shell** | 建立 `platform/log`、shell command；实现 `sys info`、`task list`、`health` 等命令。 | 串口可查询系统版本/uptime/线程。 |
| Day 6 | Zephyr 2h | **Settings + Watchdog** | 引入 settings/NVS（若内部 flash 规划尚未固定先用 RAM backend/独立分区）；看门狗喂狗与故障计数。 | 形成 `platform_health` 模块骨架。 |
| Day 7 | 复盘 1-2h | **接口设计审查** | 检查 Linux ioctl 与 Zephyr shell/service API：命名、版本、错误码、日志等级。 | 完成一次自我 code review。 |


### Week 7: 6ULL GPIO/pinctrl 实战 + MCUboot 方案设计

**周验收：第一个真实 Linux platform driver；F407 Secure Boot/DFU flash map 定稿。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **硬件反查** | 从 6ULL 原理图确认 LED/KEY 引脚；在 Reference Manual 找 IOMUX/GPIO；在 DTS 找 pinctrl。 | 记录 pin→bank→register→DTS 的完整映射。 |
| Day 2 | Linux 2h | **GPIO descriptor driver** | 用 gpiod/devm API 控 LED；禁止直接照抄寄存器版教程；状态通过 ioctl/debugfs 暴露。 | 用户态能开关 LED。 |
| Day 3 | Linux 2h | **KEY 输入** | 读按键 GPIO，处理 debounce 的基础方案；先轮询验证电平，再为 IRQ 做准备。 | 电平与示波器/万用表认知一致。 |
| Day 4 | Linux 2h | **pinctrl/clock/regulator 心智模型** | 跟踪一个现有驱动如何取得 pinctrl/clock；理解 resource managed API 与 probe failure unwind。 | 能说明 `devm_*` 解决什么问题。 |
| Day 5 | Zephyr 2h | **MCUboot Flash Map** | 按实际 F407ZGT6 sector map 设计 boot/slot0/slot1；建议学习版：Boot 128KB，slot0 384KB，slot1 512KB，并评估 swap-using-offset 的镜像上限。 | DTS fixed-partitions 初版完成；记录“镜像过大时迁移 W25Q128”的备选方案。 |
| Day 6 | Zephyr 2h | **签名与 Sysbuild** | 生成个人开发签名密钥；使用 sysbuild 构建 MCUboot+App；配置 `CONFIG_BOOTLOADER_MCUBOOT`；下载 bootloader+signed app。 | 设备只启动格式正确/签名正确镜像。 |
| Day 7 | 复盘 1-2h | **产品更新威胁模型** | 列出断电、坏包、错误版本、错误签名、升级后崩溃、重复升级等场景。 | 形成 `dfu_threat_and_failure_model.md`。 |


### Week 8: IRQ/waitqueue/poll + MCUboot/MCUmgr 串口升级

**周验收：Linux 中断驱动闭环；F407 完成第一次不接调试器的固件升级。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **IRQ context** | 复习 GIC/ARM 中断到 Linux irq subsystem；理解 hardirq/process context、禁止 sleep 原因。 | 画 IRQ 执行路径。 |
| Day 2 | Linux 2h | **request_irq/threaded IRQ** | 把 KEY 改为 IRQ；记录 `/proc/interrupts` 计数；比较 top-half 与 threaded irq。 | 按键触发中断稳定，无抖动风暴。 |
| Day 3 | Linux 2h | **waitqueue + poll** | ISR 只做最小事件记录并 wake_up；用户程序用 poll 阻塞等待。 | CPU 不再轮询；按键事件到用户态。 |
| Day 4 | Linux 2h | **workqueue/completion** | 把可睡眠工作移到 workqueue；用 completion 做一次同步实验。 | 能回答 IRQ→bottom half→process 三层职责。 |
| Day 5 | Zephyr 2h | **MCUmgr SMP UART** | 启用 mcumgr image/os management、serial transport；PC 安装客户端；验证 echo/image list。 | PC 与板通过 UART SMP 通信。 |
| Day 6 | Zephyr 2h | **升级闭环** | 构建 v1/v2 signed image；upload→image list→test→reset→confirm；日志显示版本变化。 | 不接 J-Link 完成 v1→v2。 |
| Day 7 | 复盘 1-2h | **演示脚本** | 把完整升级步骤写成 1 页 SOP；从 clean flash 重做一遍。 | 升级 SOP 可给同事执行。 |


### Week 9: Linux 并发/锁 + Firmware Rollback 故障注入

**周验收：理解真实 race 与锁选择；Secure DFU 具备回滚验证。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **制造 race** | 驱动内创建共享 counter，两个并发路径更新；压力测试产生错误值。 | 用数据证明 race，而不是只讲概念。 |
| Day 2 | Linux 2h | **mutex/spinlock** | 分别使用 mutex/spinlock 修复；在可睡眠与 atomic context 做边界分析。 | 能解释为什么不可随便互换。 |
| Day 3 | Linux 2h | **atomic/refcount/lockdep** | 用 atomic 完成适用的小计数；开启/阅读 lockdep 资料与一个简单实验。 | 形成锁选择决策表。 |
| Day 4 | Linux 2h | **ring buffer** | 实现内核 ring：producer=IRQ/work，consumer=read/poll；处理 full/empty/wrap。 | 压力测试不丢序/不越界。 |
| Day 5 | Zephyr 2h | **未确认回滚** | 上传 v2，标记 test，启动后故意不 confirm 并复位；验证 MCUboot 回退 v1。 | 保存完整 boot log。 |
| Day 6 | Zephyr 2h | **坏镜像/断电测试** | 错误 key、篡改 image、传输中断、升级过程中复位；记录系统行为和恢复方式。 | 至少完成 4 类 fault injection。 |
| Day 7 | 复盘 1-2h | **测试矩阵** | 更新 DFU 测试矩阵：precondition/action/expected/result/log。 | 形成产品测试文档雏形。 |


### Week 10: Linux 内存/MMU/mmap + Secure DFU 产品化

**周验收：建立 VA/PA/MMIO/mmap 心智模型；F407 升级流程有 Host 工具和版本策略。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **VA/PA/Page Table/TLB** | 从 Cortex-M memory map 迁移到 Linux；理解 user VA、kernel VA、PA、TLB。 | 画 3 类地址关系图。 |
| Day 2 | Linux 2h | **kmalloc/vmalloc/ioremap** | 实验 kmalloc/vmalloc；查看 `/proc/iomem`；解释 device MMIO 为什么 ioremap/readl/writel。 | 能区分内存与 MMIO。 |
| Day 3 | Linux 2h | **mmap driver** | 驱动分配 page-aligned buffer，mmap 给用户；用户验证共享数据。 | 用户态与内核共享 buffer 工作。 |
| Day 4 | Linux 2h | **内存错误调试** | 制造越界/Use-after-free 仅在 QEMU/KASAN 实验；理解 slab/page allocator 的定位工具。 | 保存 KASAN 或等价错误报告。 |
| Day 5 | Zephyr 2h | **版本与密钥策略** | 定义 semantic version/build-id、开发 key/生产 key 分离、镜像 manifest；理解 anti-downgrade/security counter。 | 写 `firmware_release_policy.md`。 |
| Day 6 | Zephyr 2h | **PC Updater CLI** | Python 实现 `probe/info/upload/test/reset/confirm` 封装，保存升级日志与 SHA256；底层可调用 mcumgr 客户端。 | `fw_update.py` 一条命令完成升级。 |
| Day 7 | 复盘 1-2h | **演示 v1→v2→rollback** | 录制文字版演示记录：成功升级、未确认回滚、坏签名拒绝。 | 项目已经可以作为简历 Demo。 |


### Week 11: DMA/Cache/Barrier + Zephyr 板级产品完善

**周验收：跨过 SoC System Software 的关键内存门槛；F407 平台可观测性完善。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **DMA 地址模型** | 理解 CPU VA→PA→DMA address；IOMMU 存在时为什么 DMA address≠PA；阅读 DMA API HOWTO。 | 画 DMA 地址路径。 |
| Day 2 | Linux 2h | **coherent vs streaming** | 比较 `dma_alloc_coherent` 与 `dma_map_single` ownership；理解 sync/map/unmap。 | 能从设备/CPU ownership 解释 cache 问题。 |
| Day 3 | Linux 2h | **6ULL DMA 现有路径** | 选 UART/SPI/SDMA 现有驱动，用源码+日志追踪 DMAEngine client/controller；不强求手写 SDMA。 | 输出真实调用链。 |
| Day 4 | Linux 2h | **barrier/cache 与 IPC** | 复习 DMB/DSB/ISB、compiler barrier、cache clean/invalidate；把概念映射到公司 IPCF。 | 写 `ipc_memory_order_notes.md`。 |
| Day 5 | Zephyr 2h | **W25Q128 选做 bring-up** | 若确认探索者 F407 板载 W25Q128：通过 SPI DTS+flash driver 读 JEDEC ID/擦写；否则改为任一现有 SPI Flash。 | 能读写外部 flash，作为未来大镜像 staging 候选。 |
| Day 6 | Zephyr 2h | **健康监控与升级状态** | Shell 增加 boot reason、active version、pending/confirmed、reset count、watchdog count。 | 一条 `fw status` 能展示设备升级状态。 |
| Day 7 | 复盘 1-2h | **System Software 口述** | 用 15 分钟解释 DMA/cache/IPC/MCUboot 的共同点：ownership、visibility、state machine。 | 更新个人知识图。 |


### Week 12: Linux 调试工具 I + Zephyr Boot/DFU 项目收口

**周验收：掌握非 printk 的内核观测工具；RTOS 产品项目形成可交付仓库。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **dynamic_debug** | 为自写驱动使用 pr_debug；通过 dynamic_debug runtime 打开/关闭日志。 | 无需重编译切换调试日志。 |
| Day 2 | Linux 2h | **ftrace function_graph** | 追踪自写 ioctl/IRQ/workqueue；设置 function filter；读懂 duration/call graph。 | 保存一份 trace。 |
| Day 3 | Linux 2h | **trace-cmd/KernelShark** | 采集 sched/irq/function events；用 KernelShark 或文本分析事件时间线。 | 解释一次 IRQ→user wakeup latency。 |
| Day 4 | Linux 2h | **Oops 深化** | 制造可控 Oops；结合 vmlinux/System.map/addr2line 定位；理解 taint、call trace。 | 独立完成定位报告。 |
| Day 5 | Zephyr 2h | **Release 包** | 整理 bootloader、signed app、host updater、keys 说明、flash map、SOP；私钥不提交公共仓库。 | 生成 `release_demo/`。 |
| Day 6 | Zephyr 2h | **系统测试** | 执行坏签名/重复版本/断电/未确认/串口中断/看门狗复位测试；统计通过率。 | 输出 `DFU_Test_Report.md`。 |
| Day 7 | 复盘 1-2h | **冻结 RTOS 项目** | 打 Git tag `v0.1-secure-dfu-demo`；写 README 架构图、构建、烧录、升级、故障恢复。 | RTOS 主项目正式收口，后续只维护。 |


### Week 13: U-Boot / Linux Boot Chain

**周验收：能够从上电到 shell 解释并实际控制 6ULL 启动链。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **Boot chain** | 画 POR→BootROM→SPL(若有)→U-Boot→Kernel→init→rootfs；对应 6ULL 实际产物。 | 每一步能指出代码/镜像/内存位置大类。 |
| Day 2 | Linux 2h | **U-Boot env/bootargs** | 操作 printenv/setenv/saveenv；理解 bootcmd、bootargs、console、root。 | 备份原环境并可恢复。 |
| Day 3 | Linux 2h | **TFTP boot** | 从网络加载 kernel+dtb；手工执行 bootz；改变 DTB 验证。 | 不依赖固定脚本启动。 |
| Day 4 | Linux 2h | **NFS root** | 配置 NFS root 或至少可替换 rootfs；故意错误 root= / IP 观察 panic。 | 能从 log 判断 rootfs 失败层级。 |
| Day 5 | Linux 2h | **early boot 调试** | 使用 earlycon/loglevel/initcall_debug（按内核支持情况）理解早期启动日志。 | 保存一次 startup timeline。 |
| Day 6 | RTOS 1h + Linux 1h | **RTOS 项目文档化** | 把 F407 board port/MCUboot 项目与 Linux bootloader 对照，重点比较 boot responsibility、image format、recovery。 | 输出 `uboot_vs_mcuboot.md`。 |
| Day 7 | 复盘 1-2h | **从“砖”恢复** | 人为设置错误 bootargs，然后仅通过 U-Boot console 修复恢复。 | 具备 BSP 基础恢复能力。 |


### Week 14: PCIe + Hailo8 枚举与资源

**周验收：把“PCIe 是扩展外设”升级成 Linux PCI subsystem 的完整模型。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux/SoC 2h | **PCIe 基础** | 掌握 RC/EP、BDF、Config Space、BAR、Memory TLP 的目的；不深挖 PHY。 | 画 RC→EP resource 图。 |
| Day 2 | Linux/SoC 2h | **Enumeration** | 公司板执行 `lspci -nn -vv -xxx`；找到 Hailo Vendor/Device/BAR/Link speed/width。 | 保存基线枚举报告。 |
| Day 3 | Linux/SoC 2h | **Linux PCI driver** | 跟踪 `pci_driver/id_table/probe/pci_enable_device/pci_request_regions/pci_iomap`。 | 定位 Hailo probe 入口。 |
| Day 4 | Linux/SoC 2h | **MSI/MSI-X/DMA** | 从 `/proc/interrupts` 看中断；理解 bus master、DMA mask、MSI。 | 能解释设备如何主动写 Host 内存。 |
| Day 5 | Linux/SoC 2h | **异常定位** | 构造/收集 link down、driver 不匹配、BAR 资源冲突类诊断步骤；不破坏生产环境。 | 形成 PCIe 排障 checklist。 |
| Day 6 | RTOS 1h + Linux 1h | **知识复用** | 比较 F407 MCUboot flash map 与 PCI BAR/resource map：都是“资源描述+消费者”，但机制不同。 | 写 1 页知识迁移笔记。 |
| Day 7 | 复盘 1-2h | **面试口述** | 20 分钟白板：Enumeration→probe→BAR→IRQ→DMA。 | 做到不看源码能讲完整。 |


### Week 15: Hailo8 Driver → HailoRT 调用链

**周验收：当前工作项目从“移植成功”升级为“能解释 Runtime/Driver/PCIe 数据面”。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux/SoC 2h | **用户态反向跟踪** | 对最小 Hailo demo 使用 strace；记录 open/ioctl/mmap/poll/threads。 | 得到 syscall 清单。 |
| Day 2 | Linux/SoC 2h | **UAPI 映射** | 从 device node 找 file_operations；把主要 ioctl 映射到 handler。 | 制作 ioctl call map。 |
| Day 3 | Linux/SoC 2h | **Memory path** | 追 mmap/DMA buffer allocation；区分 control path 与 data path。 | 画 Host buffer 生命周期。 |
| Day 4 | Linux/SoC 2h | **IRQ/DMA path** | 定位 interrupt handler/workqueue/completion；找 DMA 提交与完成路径。 | 能说清一次 inference 的“通知机制”。 |
| Day 5 | Linux/SoC 2h | **ftrace/动态日志** | 使用 ftrace/dynamic debug 对关键函数采样；验证你画的调用链。 | 用 trace 证实而非仅靠阅读。 |
| Day 6 | Linux/SoC 2h | **故障模型** | 总结 device missing、firmware mismatch、timeout、DMA fail、IRQ miss 的观测点。 | 输出 `hailo_debug_playbook.md`。 |
| Day 7 | 复盘 1-2h | **项目讲解** | 写 3 分钟/10 分钟两个版本项目介绍。 | 开始具备面试项目故事。 |


### Week 16: Yocto/Hailo 工程化集成

**周验收：能解释 BitBake、CMake、Kernel module、Runtime 依赖和离线构建边界。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Yocto 2h | **任务流水线** | 复盘 fetch/unpack/patch/configure/compile/install/package/rootfs；用 `bitbake -e/-c` 观察真实任务。 | 画 Hailo recipe task graph。 |
| Day 2 | Yocto 2h | **关键变量** | 针对当前 recipe 查 `${S}/${B}/${D}/WORKDIR/DL_DIR/SSTATE_DIR`。 | 能从 log 快速定位阶段。 |
| Day 3 | Yocto 2h | **依赖** | 用当前 Hailo 案例区分 DEPENDS/RDEPENDS/native/nativesdk；查看 pkgdata。 | 输出依赖表。 |
| Day 4 | Yocto 2h | **Kernel module packaging** | 跟踪 `.ko` 编译、modules_install、package、autoload/service；确认最终 image 路径。 | 能独立把一个 module 打进镜像。 |
| Day 5 | Yocto 2h | **离线构建机制** | 拆分 BitBake fetch 与 CMake FetchContent；整理你当前 16 个 extern 依赖的最小闭包和失败点。 | 形成公司可复用离线构建说明。 |
| Day 6 | Yocto 2h | **从 clean state 重做** | 清理一个受控 recipe 状态并重新构建；保存完整证据。 | 避免“偶然构建成功”。 |
| Day 7 | 复盘 1-2h | **面试化表达** | 用“为什么内网失败”作为系统设计题回答：source acquisition/build dependency/runtime package。 | 形成 10 分钟回答。 |


### Week 17: 异构多核 IPC：从调通提升到系统设计

**周验收：现有 IPCF 项目上升为高级 SoC System Software 项目。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | SoC 2h | **数据结构** | 重新阅读 IPC-SHM/IPCF，只追 descriptor/ring/index/shared region；画 producer/consumer。 | 不再泛读源码。 |
| Day 2 | SoC 2h | **Cache visibility** | 确认 A53/MCU cache 属性、clean/invalidate 路径；找代码证据或平台文档。 | 回答“写了为何对端看不到”。 |
| Day 3 | SoC 2h | **Memory ordering** | 找 barrier/atomic/lock；区分 data visibility 与 ordering。 | 完成一个具体 race scenario 分析。 |
| Day 4 | SoC 2h | **Notification** | 追 mailbox/IPI/IRQ：谁触发、谁 ack、何时 drain ring。 | 画完整时序。 |
| Day 5 | SoC 2h | **Recovery** | 分析 remote reset、timeout、ring corruption、version mismatch；提出 state machine。 | 形成 recovery 设计表。 |
| Day 6 | SoC 2h | **Instrumentation** | 增加不会破坏产品的计数/trace：tx/rx/drop/ring high-water/IRQ/timeout。 | 能量化而非只看功能通过。 |
| Day 7 | 复盘 1-2h | **IPCF 架构文档** | 完成 `IPCF_architecture.md` 第一版。 | 可用于面试白板。 |


### Week 18: 性能与高级调试 II

**周验收：掌握 perf/ftrace 的工程使用，并完成 IPC/Hailo 的量化分析。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | Linux 2h | **perf stat/top** | 对测试程序/Hailo demo 看 cycles/instructions/context-switch/cache-miss；解释指标，不追求绝对值。 | 保存 baseline。 |
| Day 2 | Linux 2h | **perf record/report** | 采样热点；生成调用栈；验证是否有不必要 copy/spin。 | 完成热点分析报告。 |
| Day 3 | Linux 2h | **sched/irq trace** | 用 trace-cmd/ftrace 看 irq/sched/wakeup；测一次端到端事件延迟。 | 获得时间线证据。 |
| Day 4 | SoC 2h | **IPC benchmark** | 按消息 64B/256B/1KB/4KB 测 latency/throughput/CPU/IRQ。 | 生成 benchmark 表。 |
| Day 5 | SoC 2h | **压力与可靠性** | 长时间、多通道、ring full、remote reset；Python 自动收集日志和统计。 | 形成 reliability report。 |
| Day 6 | QEMU 2h | **kgdb/crash 入门** | QEMU 上至少完成一次 kgdb 断点或 crash/vmcore 基本分析；理解 KASAN/lockdep/pstore 使用场景。 | 不要求精通，但必须亲自做一次。 |
| Day 7 | 复盘 1-2h | **Linux Debug Playbook** | 整理“问题→首选工具→证据→下一步”的调试矩阵。 | 输出 `Linux_Debug_Playbook.md`。 |


### Week 19: 项目收口 + Coding 恢复

**周验收：四个项目故事可面试；恢复必要手写代码能力。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | 项目 2h | **项目一：异构 IPC** | 按背景/难点/行动/证据/结果组织 IPCF；重点 cache、barrier、recovery、benchmark。 | 3 分钟版本完成。 |
| Day 2 | 项目 2h | **项目二：Hailo8** | 组织 Yocto+PCIe+Driver+Runtime+离线构建故事。 | 能承受 20 分钟追问。 |
| Day 3 | 项目 2h | **项目三：6ULL Linux Lab** | 整理 DTS/platform/VFS/IRQ/mmap/DMA/debug/boot 的实验证据。 | Git README 可公开或脱敏。 |
| Day 4 | 项目 2h | **项目四：Zephyr Secure DFU** | 整理 board port、MCUboot、signed image、rollback、host updater、fault injection。 | 项目描述不超过 5 行但每行可深挖。 |
| Day 5 | Coding 2h | **基础结构** | 手写 list/queue/ring/hash/binary search；必须在无 IDE 补全条件下写。 | 5 题计时完成。 |
| Day 6 | Coding 2h | **树/位/内存** | BST/DFS/BFS/bitmap/memmove/endian；复习生产者消费者。 | 补齐此前弱项。 |
| Day 7 | 模拟 2h | **第一次完整技术面** | 60 分钟：自我介绍+项目+Linux driver+IPC+coding；记录卡点。 | 生成 gap list。 |


### Week 20: 面试定型与正式投递

**周验收：达到“高级 SoC System Software”可投递状态，计划从学习转为面试反馈闭环。**

| Day | 时间/主线 | 当天目标 | 学习与实操细节 | 当天验收 |
|---|---|---|---|---|
| Day 1 | 面试 2h | **Linux Driver 高频** | 白板 DTS→probe、fd→fops、IRQ→poll、mmap；随机手写驱动骨架。 | 每题 5 分钟内形成结构化答案。 |
| Day 2 | 面试 2h | **Memory/DMA/Cache** | 集中回答 VA/PA/ioremap/DMA API/coherent/barrier/cache；结合 IPCF/Hailo。 | 避免只背定义。 |
| Day 3 | 面试 2h | **Boot/PCIe/Yocto** | BootROM→U-Boot→Kernel；PCI enum→BAR→DMA；Yocto task/dependency。 | 三个 10 分钟白板题。 |
| Day 4 | 面试 2h | **RTOS/Zephyr/MCUboot** | FreeRTOS kernel、Zephyr device model/Kconfig/DTS、MCUboot state/rollback/签名；说明为何做此项目。 | 能把 RTOS 讲成 platform 而不是 API。 |
| Day 5 | 系统设计 2h | **两个系统设计题** | 题 1：A53+M核 IPC；题 2：远程设备安全固件升级。要求异常/恢复/监控/测试。 | 输出两张架构图。 |
| Day 6 | 求职 2h | **简历/JD 映射** | 针对 3 类岗位各做一版摘要：SoC Platform、Linux BSP/Driver、AI Accelerator System Software；开始正式投递。 | 每个 JD 10 分钟完成能力映射。 |
| Day 7 | 复盘 2h | **重新画能力拼图** | 对照 Week 1 拼图重新评分；红色项必须有实验证据或真实项目支撑；未补齐项进入面试反馈迭代。 | 20 周项目结束，转入投递/面试循环。 |


## 6. 四个阶段 Gate：没通过就不要往后赶

### Gate A - Week 5

- 6ULL：能 clean build kernel/DTB/module；能解释 DTS→platform_device→match→probe。
- F407：自定义 Zephyr board 可 build/flash/debug；LED/KEY/console 正常。

### Gate B - Week 10

- Linux：`IRQ → waitqueue → poll → user` 实验完成；能写 ioctl/mmap；锁选择有真实 race 证据。
- Zephyr：v1→v2 Secure DFU、未确认回滚、坏签名拒绝全部完成；Host updater 可用。

### Gate C - Week 15

- Linux：U-Boot 网络启动与启动故障定位完成；DMA/cache 地址模型讲得清。
- Hailo：`App → HailoRT → ioctl/mmap → Driver → PCIe/DMA/IRQ → Hailo8` 有源码/trace 证据。

### Gate D - Week 20

- 能用 20 分钟白板解释：Linux Driver、DMA/cache、PCIe、异构 IPC、MCUboot 安全升级。
- 至少四个项目故事中有三个可被面试官连续深挖 15–20 分钟。

## 7. 学习内容优先级：防止再次学散

| 优先级 | 内容 | 深度要求 |
|---|---|---|
| P0 | DTS / Driver Model / VFS / IRQ / mmap | 必须能写、能调、能口述 |
| P0 | DMA / cache / barrier / PCIe | 必须形成系统心智模型并连接 IPCF/Hailo |
| P0 | ftrace / perf / strace / gdb / addr2line | 必须亲自使用 |
| P0 | Zephyr DTS/Kconfig/device model + MCUboot | 必须完成产品型项目 |
| P1 | workqueue/completion/debugfs/sysfs/KASAN/lockdep/kgdb | 至少做一次实际实验 |
| P1 | W25Q128 staging / MCUmgr UDP | 时间允许再做 |
| P2 | LVGL/BLE/USB/文件系统深入 | 当前不学，JD 要求再补 |
| P2 | AI Runtime/MLIR/TVM | 本 20 周明确不展开 |

## 8. 面试用最终能力模型

```text
                       SoC System Software
                               │
     ┌─────────────────────────┼─────────────────────────┐
     │                         │                         │
 MCU / RTOS               Linux / BSP              Accelerator
     │                         │                         │
 Zephyr/FreeRTOS          U-Boot / DTS             PCIe / Hailo8
 MCUboot / DFU            Driver / IRQ             Runtime / DMA
     │                         │                         │
     └──────────── Shared Memory / Cache / Barrier ─────┘
                               │
                         Debug / Performance
                JTAG + ftrace + perf + automation
```

## 9. 官方资料与参考入口

- [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- [Zephyr STM32F4 Discovery](https://docs.zephyrproject.org/latest/boards/st/stm32f4_disco/doc/index.html)
- [Zephyr Build/Flash/Debug](https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html)
- [Zephyr DFU](https://docs.zephyrproject.org/latest/services/device_mgmt/dfu.html)
- [Zephyr MCUmgr](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html)
- [MCUboot with Zephyr](https://docs.mcuboot.com/readme-zephyr.html)
- [MCUboot Design](https://docs.mcuboot.com/design.html)
- [Linux Platform Driver](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux Tracing](https://docs.kernel.org/trace/)
- [Linux KGDB/KDB](https://docs.kernel.org/process/debugging/kgdb.html)
- [U-Boot Environment](https://docs.u-boot.org/en/latest/usage/environment.html)
- [U-Boot Standard Boot](https://docs.u-boot.org/en/latest/develop/bootstd/index.html)

> 正点原子 F407 板级资源在开源 RT-Thread BSP 中也有成熟描述，可用于交叉校验原理图与外设资源；最终板级 DTS 仍以你手上开发板的原理图、PCB 版本和 MCU 丝印为准。
