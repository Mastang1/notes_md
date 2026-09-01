# Linux Kernel / Driver 系统开发学习指导
## —— 面向 MCU / RTOS / ARM 硬件背景工程师的 6 个月 Level 3 路线

**版本**：V1.1（专家审校增强版）  
**日期**：2026-09-01  
**学习周期**：24 周  
**建议投入**：2 h/天，约 14 h/周，总计约 330–340 h  
**核心目标**：独立开发、移植、调试 Linux 驱动；具备外部硬件模块选型后完成 Linux 集成与驱动开发的能力  
**重点方向**：PCIe > Linux Kernel 核心机制 > Driver/BSP  
**主实验环境**：QEMU + Linux 6.18 LTS + NXP i.MX6ULL 实板（正点原子开发板）  
**辅助环境**：产品实际 vendor kernel / BSP；后期可加入真实 PCIe 平台

---

# 0. 文档定位

这不是一份“Linux 内核知识大全”，也不是按教材章节从头到尾阅读的计划。

它面向已经具备以下能力的工程师：

- 熟悉 MCU / 裸机 / RTOS 开发；
- 熟悉 C、指针、函数指针、并发基本概念；
- 熟悉 ARM IP、MMIO、IRQ、DMA 等硬件概念；
- 能看 datasheet、reference manual、原理图并进行硬件调试；
- 已经接触过简单 Linux driver 调试；
- 对 Linux Driver Model、VFS 有初步框架性认识，但尚未从 0 到 1 完整负责过 Linux 驱动开发。

因此，本路线**刻意跳过大量低收益入门内容**，把有限的 6 个月集中到真正决定 Linux Driver 工程能力的知识链：

```text
硬件行为
   ↓
Linux 对硬件的抽象
   ↓
Device / Bus / Driver / Subsystem
   ↓
资源管理：MMIO / IRQ / Clock / Reset / DMA
   ↓
并发与生命周期
   ↓
驱动数据路径
   ↓
Linux 调试与 tracing
   ↓
真实 vendor driver / 外部模块集成
```

最终能力不以“学过多少章节”衡量，而以：

> 面对陌生模块、陌生 driver、陌生 SoC，能否建立正确模型、快速找到关键路径、完成 bring-up，并在发生故障时进行分层定位。

---

# 1. 专家评估：你的真实起点与最优路线

## 1.1 你的优势

你的 MCU / RTOS / ARM 背景已经解决了 Linux Driver 学习中很多初学者最困难的问题：

1. **硬件抽象不是陌生概念**  
   寄存器、bit field、IRQ、DMA、时钟、reset、总线时序等不需要从头学。

2. **已经具备“执行上下文”意识**  
   RTOS 中 ISR/task、mutex/semaphore/queue 的经验可以迁移到 Linux 的 process context、hardirq、workqueue、completion、wait queue。

3. **C 和底层调试能力较强**  
   可以直接进入真实源码，而不需要长期停留在字符设备模板。

4. **有 IP / SoC 视角**  
   这非常适合后续理解：
   - Device Tree；
   - clock/reset/pinctrl；
   - PCIe Host Controller；
   - DesignWare PCIe；
   - DMA/IOMMU；
   - vendor glue driver。

## 1.2 主要能力缺口

真正要补的不是“Linux API 数量”，而是以下五个映射：

### 缺口 A：硬件对象 → Linux 对象

```text
SoC UART
  ↓
DT node
  ↓
platform_device
  ↓
struct device
  ↓
platform_driver
  ↓
probe()
```

### 缺口 B：硬件资源 → Linux resource framework

```text
寄存器地址  → struct resource → ioremap/devm_ioremap_resource
IRQ line   → irq domain        → Linux IRQ number
Clock      → common clock framework
Reset      → reset framework
DMA        → DMA API / IOMMU / dma_addr_t
```

### 缺口 C：driver 生命周期

```text
device discovery
→ match
→ probe
→ active data path
→ suspend/resume
→ remove
→ resource release
```

### 缺口 D：并发与上下文

不能只知道“有 spinlock/mutex”，而要形成：

```text
谁和谁并发？
各自是什么 execution context？
共享什么状态？
谁拥有数据？
谁能 sleep？
什么时候需要 barrier？
```

### 缺口 E：系统化调试

从：

```text
加 printk → 重编 → 再看
```

升级为：

```text
定义故障层级
→ 收集证据
→ dynamic_debug / tracepoint / ftrace
→ kprobe / perf
→ sanitizer / lockdep
→ GDB / crash dump
```

---

# 2. 一个关键结论：工业界 driver 开发通常不是“从空白写 driver”

你的判断是正确的。

实际 Linux Driver 工作通常可以分为四种模式。

## 模式 1：已有 upstream driver

最理想。

工作重点往往是：

- DTS / DT binding；
- compatible；
- clock/reset/pinctrl；
- regulator；
- IRQ；
- DMA；
- kernel config；
- subsystem 集成；
- board-specific 差异。

此时**原则上不要 fork 一个私有 driver 重写**。

---

## 模式 2：厂商提供 vendor driver，但内核版本不同

这是工业场景中极常见的一类问题。

例如：

```text
vendor driver: Linux 5.x
product kernel: Linux 6.x
```

可能出现：

- kernel API 已变化；
- timer/workqueue API 变化；
- proc/sysfs 接口变化；
- DMA API 或 helper 变化；
- class/device API 变化；
- PCI / MSI helper 变化；
- PM API 变化；
- deprecated API 被删除。

Linux 内核**不承诺稳定的 in-kernel driver ABI/API**，因此 driver porting 是核心能力，而不是例外。

---

## 模式 3：没有本芯片 driver，但有类似芯片 driver

这是非常典型的“驱动开发”。

例如同一厂商：

```text
ADC-A 已 upstream
ADC-B 新产品未 upstream
```

工程上通常不是完全从 0 开始，而是：

```text
datasheet diff
+ similar upstream driver
+ register map
+ device-specific quirks
```

这种方式通常比自己重新发明 framework 更可靠。

---

## 模式 4：完全自研硬件 / FPGA / ASIC

这才是真正的：

```text
datasheet/register spec
→ subsystem design
→ Linux driver architecture
→ MMIO
→ IRQ
→ DMA
→ userspace/kernel interface
→ PM
→ error recovery
→ testing
```

本学习路线最终会让你有能力处理这四种情况。

---

# 3. Level 3 能力定义

完成 24 周后，应达到以下状态。

## 3.1 必须能够独立完成

### Driver bring-up

- 判断设备属于什么 Linux subsystem；
- 找 upstream 是否已有支持；
- 判断 vendor driver 是否值得移植；
- 读 DT；
- 分析 probe；
- 验证 MMIO；
- 验证 IRQ；
- 验证 DMA；
- 验证 userspace 接口；
- 做基本稳定性测试。

### Driver debug

面对：

```text
probe 不执行
probe 返回错误
MMIO 读异常
IRQ 不来
DMA timeout
DMA 数据偶发错误
驱动随机 crash
卸载/重载 crash
SMP 下偶发异常
PCI BAR map 失败
IOMMU fault
```

能形成有依据的排查树。

### Vendor driver porting

给一个旧 driver：

- 找 API breakage；
- 找版本差异；
- 修改；
- 编译；
- 静态检查；
- 上板；
- 验证；
- 做回归。

### 外部模块选型 + 驱动风险评估

产品提出：

> 需要一个 PCIe / SPI / I2C / USB 外部模块。

你应能在采购前判断：

- Linux 支持程度；
- upstream 状态；
- vendor driver 质量；
- 依赖的 kernel 版本；
- 固件依赖；
- DMA / IRQ 模型；
- porting 风险；
- 预计开发难度；
- 长期维护风险。

---

# 4. Linux Driver 的第一性知识地图

以后看到任何 driver，都先套以下五层模型。

```text
┌────────────────────────────────────────────┐
│               Userspace / Product          │
│ app / daemon / ioctl / mmap / sysfs / ... │
├────────────────────────────────────────────┤
│               Linux Subsystem              │
│ net / input / hwmon / IIO / DRM / ALSA... │
├────────────────────────────────────────────┤
│               Driver Model                 │
│ device / driver / bus / class / probe / PM│
├────────────────────────────────────────────┤
│           Controller / Bus Layer           │
│ PCIe RC / SPI host / I2C adapter / USB    │
├────────────────────────────────────────────┤
│                  Hardware                  │
│ MMIO / IRQ / DMA / clock / reset / PHY    │
└────────────────────────────────────────────┘
```

## 4.1 阅读 driver 时固定回答 12 个问题

1. 这个设备怎么被发现？
2. 谁创建 `struct device`？
3. 它属于哪个 bus？
4. driver 与 device 根据什么 match？
5. `probe()` 从哪里被调用？
6. driver 私有数据放在哪里？
7. MMIO/resource 从哪里获得？
8. IRQ 如何获得和注册？
9. 数据路径在哪里？
10. DMA buffer 谁申请、谁拥有、何时释放？
11. userspace 通过什么标准接口使用设备？
12. remove/error path 是否与 probe 对称？

如果不能回答这些问题，就不能算真正读懂了 driver。

---

# 5. 学习环境设计

原则：

> **环境服务于知识，不让知识绑死在开发板上。**

---

## 5.1 环境 A：QEMU —— 机制学习与故障注入主平台

建议 host：

- Ubuntu 24.04 LTS 或其他稳定 Linux workstation；
- GCC；
- Clang/LLVM；
- GDB；
- QEMU；
- Git；
- ripgrep；
- ctags / clangd；
- cscope（可选）；
- trace-cmd；
- perf；
- bpftrace（后期）。

主学习 kernel：

```text
Linux 6.18.y LTS
```

同时保留：

```text
Linux 6.12.y LTS
```

用于版本差异对照。

截至 2026-09，6.18 和 6.12 都处于 longterm maintenance；学习不需要追最新 RC。

### QEMU 的用途

- kernel build / boot；
- GDB 单步；
- crash；
- KASAN；
- KCSAN；
- lockdep；
- ftrace；
- driver model；
- 虚拟设备；
- PCI EDU；
- DMA；
- fault injection。

---

## 5.2 环境 B：NXP i.MX6ULL —— 真实 SoC / BSP 训练

主要训练：

- Device Tree；
- platform driver；
- GPIO；
- pinctrl；
- clock；
- reset；
- regulator；
- I2C；
- SPI；
- IRQ；
- real MMIO；
- 示波器 / 逻辑分析仪联合调试。

NXP i.MX6ULL Reference Manual 作为硬件权威资料。

注意：

> 正点原子 BSP 使用什么 kernel，就把它当作“vendor/legacy track”，不要强行要求它与 6.18 完全一致。

这样反而可以训练真实的 kernel version gap。

---

## 5.3 环境 C：QEMU PCI EDU —— PCIe 软件栈教学平台

QEMU 自带：

```bash
-device edu
```

它本身就是为了练习 kernel driver 而设计的 PCI 教学设备，提供：

- PCI enumeration；
- BAR/MMIO；
- IRQ；
- DMA；
- DMA mask。

因此学习 PCI 时不必等待真实 PCIe 开发板。

后续再把同一模型迁移到：

- Hailo；
- FPGA PCIe 卡；
- NIC；
- NVMe；
- 实际 SoC RC。

---

# 6. 学习方法：不是“看懂”，而是形成迁移能力

## 6.1 30 / 40 / 30 原则

长期比例建议：

```text
30% 原理 / 文档
40% 源码 / 调用链
30% 实验 / 调试
```

某些周可以调整，但不允许长期变成：

```text
90% 看教程
10% 实验
```

Bootlin 当前 Linux kernel driver 课程本身也把约 50%–60% 时间安排为实践实验，说明 driver 能力本质上必须通过 hands-on 形成。

---

## 6.2 Scenario-Based Source Reading

不要“通读 Linux”。

以问题驱动。

例如：

> 为什么 platform driver 的 `probe()` 被调用？

只跟这一条路径。

代表性思路：

```text
platform_driver_register()
        ↓
driver core registration
        ↓
bus matching
        ↓
device-driver binding
        ↓
probe()
```

具体内部函数随 kernel 版本可能变化，因此学习时必须自己：

```bash
rg "platform_driver_register"
rg "driver_register"
git log -S "function_name"
```

验证当前版本。

### 停止条件

当你能画出：

1. 核心 object；
2. sequence；
3. match 条件；
4. failure point；

就停止继续向下钻。

---

## 6.3 Retrieval Practice：闭卷回忆

学习开始前 10–20 分钟，不看资料回答旧问题。

例如：

- `device` 和 `driver` 是谁触发 match？
- 为什么 `probe()` 不执行？
- `dma_addr_t` 是 CPU physical address 吗？
- hardirq 为什么不能 sleep？
- `BAR` 是谁分配的？
- `ioremap()` 得到什么地址？

检索练习相较于单纯重读，在长期保持以及一定程度的迁移上有可靠研究支持。

---

## 6.4 Spacing：重复必须跨时间

本课程使用一个**工程上简单可执行的经验节奏**：

```text
D0  学习
D+1 闭卷回忆
D+3 小实验
D+7 周测
D+21 跨主题复测
```

这不是“认知科学规定的唯一最优间隔”，而是结合 spacing evidence 与工程日程设计的可执行模板。

---

## 6.5 Interleaving：相似知识交叉比较

不要：

```text
I2C 学完 → 永不再碰
SPI 学完 → 永不再碰
PCI 学完
```

要反复比较：

```text
platform probe
vs I2C probe
vs SPI probe
vs PCI probe
```

以及：

```text
DT reg
vs PCI BAR
```

```text
platform IRQ
vs MSI
```

```text
SPI controller driver
vs SPI device driver
```

Interleaving 的研究表明，在需要区分类似类别时特别有价值；Driver Model 中的多 bus 对比正适合这种学习方式。

---

## 6.6 Worked Example → Fading

前期：

```text
完整示例
→ 带提示的实验
→ 只给目标
→ 完全陌生 driver
```

不能从第一周就“纯硬啃整个 kernel”，也不能半年都跟着教程复制。

你的背景已经较强，因此 guidance fading 要比普通初学者更快。

---

# 7. 每天 2 小时的固定模板

## 标准工作日

### 0–15 min：闭卷 retrieval

写答案，不查资料。

### 15–45 min：新知识

只学一个核心问题。

### 45–100 min：源码 / 实验

这是主区。

### 100–115 min：故障注入 / 变式

例如：

```text
故意写错 compatible
故意不 enable clock
故意漏 dma_unmap
故意把 mutex 放到 IRQ
```

观察 Linux 如何失败。

### 115–120 min：写 3 行日志

只回答：

1. 今天新建立了什么模型？
2. 今天哪里判断错了？
3. 下次第一件事是什么？

---

# 8. 每周学习节奏

```text
Day 1  原理 + 最小实验
Day 2  源码调用链
Day 3  实板 / QEMU 实验
Day 4  故障注入与调试
Day 5  阅读一个真实 upstream driver
Day 6  周项目
Day 7  闭卷验收 + 笔记整理
```

不要追求每天“学新东西”。

**Day 7 不新增知识。**

---

# 9. 24 周总体路线

| 阶段 | 周次 | 核心主题 | 阶段产物 |
|---|---:|---|---|
| Phase 0 | W1–W2 | Kernel 开发闭环与源码导航 | 可随意修改/编译/启动/调试 kernel |
| Phase 1 | W3–W6 | 执行上下文 + Driver Model + DT + Platform | 完整 platform MMIO/IRQ driver |
| Phase 2 | W7–W10 | BSP资源 + I2C/SPI + Subsystem + 模块选型 | 实板外设 driver + Integration Report |
| Phase 3 | W11–W14 | IRQ/并发/MM/DMA | DMA 与并发模型建立 |
| Phase 4 | W15–W19 | PCIe 完整专项 | QEMU EDU PCI driver + Host/DesignWare 分析 |
| Phase 5 | W20–W22 | Kernel Debugging | tracing / sanitizer / GDB 故障定位 |
| Phase 6 | W23–W24 | Vendor porting + 综合项目 | Level 3 毕业项目 |

---

# 10. Phase 0：W1–W2 —— Kernel 开发闭环

## W1：Build → Boot → Modify

### 目标

建立：

```text
改源码
→ 编译
→ QEMU 启动
→ 验证
→ 再修改
```

低摩擦闭环。

### 必学

- kernel source tree；
- `make defconfig`；
- `menuconfig`；
- out-of-tree build；
- `vmlinux`；
- compressed image；
- symbols；
- initramfs / rootfs 基本关系。

### 实验

1. 编译 Linux 6.18.y；
2. QEMU 启动；
3. 修改一个明确可执行的位置；
4. 验证新 kernel；
5. 保存 boot script。

### 验收

不看教程，30 分钟内：

- 清理 build；
- 重编；
- QEMU boot；
- 找到 `vmlinux`；
- GDB 能加载 symbols。

---

## W2：Kbuild / Module / Source Navigation

### 必学

```text
Kconfig
Makefile
Kbuild
CONFIG_*
module_init
module_exit
EXPORT_SYMBOL
modprobe
insmod
lsmod
modinfo
kallsyms
```

### 工具

```bash
rg
git grep
git log
git blame
git log -S
git log -G
ctags / clangd
```

### 实验

写一个最小 module，但目的不是学习“Hello World”。

目的：

- 理解模块如何进入 kernel；
- 理解 symbol resolution；
- 理解 module lifecycle；
- 学会从宏跳到实际定义。

### Milestone M0

拿到任意 kernel API，能：

1. 找声明；
2. 找定义；
3. 找调用者；
4. 找最近历史；
5. 判断 API 是否版本相关。

---

# 11. Phase 1：W3–W6 —— Driver Model 主干

## W3：Linux Execution Context

你的 RTOS 知识要在这周完成迁移。

### 建立映射

| RTOS 思维 | Linux 对应 |
|---|---|
| task/thread | process / kernel thread |
| ISR | hardirq |
| deferred ISR | threaded IRQ / workqueue / softirq |
| semaphore | semaphore/completion/mutex（语义不同） |
| critical section | spinlock/mutex/atomic 等 |
| scheduler lock | Linux 中通常不应照搬这种设计 |

### 必须搞清

- process context；
- interrupt context；
- preemption；
- sleep；
- atomic context；
- kernel stack；
- syscall；
- workqueue。

### 验收

给 20 个 kernel API，判断：

> 当前 context 能不能调用？

---

## W4：Linux Driver Model

### 核心 object

```c
struct device
struct device_driver
struct bus_type
struct class
struct kobject
```

但不背字段。

要理解关系：

```text
bus
├── devices
└── drivers

device ←bind→ driver
```

以及：

```text
/sys/devices
/sys/bus
/sys/class
/sys/module
```

### 必跟调用场景

```text
device register
→ match
→ bind
→ probe
```

### 故障实验

人为制造：

- id 不匹配；
- driver 不注册；
- device 不存在；
- probe 返回错误。

### 验收题

> driver 已成功 `insmod`，但 `probe()` 没打印，怎么查？

必须给出分层方法，而不是“多加 printk”。

---

## W5：Device Tree + Platform Bus

### 必学 DT 属性

```text
compatible
reg
interrupts
clocks
resets
dmas
pinctrl-*
*-gpios
status
```

### 核心模型

```text
DT node
   ↓
device enumeration
   ↓
platform_device
   ↓
of_match
   ↓
platform_driver
   ↓
probe()
```

### 注意

Device Tree 不是“Linux 配置文件”。

它本质上是 firmware/bootloader 提供给 OS 的**硬件描述数据结构**。

### 实验

在 QEMU 或 i.MX6ULL：

- 改 compatible；
- 改 status；
- 改 reg；
- 观察 device / probe 行为。

---

## W6：第一个完整 Platform Driver

### 目标

完成：

```text
platform
+ MMIO
+ IRQ
```

### API 类别

```c
platform_get_resource()
devm_ioremap_resource()
platform_get_irq()
devm_request_irq()
platform_set_drvdata()
```

具体 API 以当前 kernel 文档/源码为准。

### 强制要求

driver 必须有：

- probe；
- remove；
- error handling；
- resource ownership；
- runtime state struct。

### Milestone M1

独立写出一个小型 platform driver，并能解释：

```text
设备从哪里来
为什么 match
为什么 probe
reg 从哪里来
IRQ 从哪里来
资源何时释放
```

---

# 12. Phase 2：W7–W10 —— 实板 BSP 与常见外设

## W7：Clock / Reset / Pinctrl / GPIO / Regulator

这是实际 BSP 中极高频的故障源。

### 建立启动依赖链

```text
power
→ reset
→ clock
→ pinmux
→ MMIO
→ peripheral state
→ IRQ
```

### 故障注入

分别故意：

- 不开 clock；
- 不 release reset；
- pinctrl 错；
- GPIO active level 错。

### 目标

以后碰到：

> register 全 0 / 总线没波形 / IRQ 不来

不会第一时间认定 driver algorithm 有 bug。

---

## W8：I2C + regmap

### 学习方法

选择一个真实 I2C 芯片。

流程：

```text
datasheet
→ address
→ register map
→ upstream 搜索
→ subsystem
→ DT binding
→ driver
```

### 核心

```c
struct i2c_driver
struct i2c_client
regmap
```

### 重点

学习 `regmap` 的原因：

- register abstraction；
- endian；
- cache；
- debug；
- bus independence。

### 验收

不参考模板，从 datasheet 创建最小 driver。

---

## W9：SPI + IRQ

### 重点不是重复 I2C

重点是对比：

```text
platform_device
i2c_client
spi_device
```

其共同结构：

```text
device
→ bus
→ match
→ probe
→ resource
→ subsystem
```

### 实验

最好选择：

- SPI ADC；
- SPI sensor；
- 简单 SPI peripheral。

加入一个 IRQ/Data Ready。

---

## W10：Subsystem + 外部模块选型演练

这里开始脱离“字符设备万能论”。

### 必须建立原则

如果已有标准 subsystem：

> 优先进入标准 subsystem，不要为了方便随手做 `/dev/mydev + ioctl`。

例如：

- sensor → IIO / hwmon；
- network → netdev；
- input → input subsystem；
- RTC → RTC；
- GPIO → gpiolib；
- PWM → PWM；
- watchdog → watchdog；
- serial → TTY/serdev；
- storage → block/SCSI/NVMe 等。

### 项目

模拟公司采购模块。

输出：

`driver_integration_report.md`

包括：

1. 产品需求；
2. 模块接口；
3. upstream 支持；
4. vendor driver；
5. kernel version；
6. DT；
7. IRQ；
8. DMA；
9. firmware；
10. userspace；
11. license；
12. 风险；
13. 预计开发工作量。

### Milestone M2

能在**一天内**对陌生模块形成 Linux 集成可行性结论。

---

# 13. Phase 3：W11–W14 —— IRQ / 并发 / 内存 / DMA

## W11：Interrupt + Deferred Work

### 必须区分

```text
hardirq
threaded irq
softirq
workqueue
timer
tasklet（理解 legacy / existing code）
```

### 核心问题

不是“API 怎么用”，而是：

> 为什么这部分代码必须从 hardirq 移到可 sleep context？

### 实验

1. IRQ handler；
2. threaded IRQ；
3. workqueue；
4. 故意在错误 context 做 sleep；
5. 看 warning / lockdep。

---

## W12：Synchronization

### 核心 primitive

```text
spinlock
mutex
atomic
completion
wait queue
semaphore
RCU（只建立模型）
```

### 每次使用锁前写 5 行

```text
并发主体：
共享对象：
context：
是否允许sleep：
ownership：
```

### 学习重点

- race；
- deadlock；
- lock ordering；
- lifetime race；
- interrupt vs process；
- SMP。

### 实验

人为制造：

- ABBA deadlock；
- missing lock；
- use-after-free race。

---

## W13：Memory / MMIO / Barrier

不全面学习 Linux MM。

只学 driver 必需部分：

```text
kernel VA
CPU PA
bus address
page
kmalloc
vmalloc
alloc_pages
ioremap
MMIO
GFP_KERNEL
GFP_ATOMIC
```

### 必须区别

```text
normal memory
vs
device memory
```

以及：

```text
compiler ordering
CPU ordering
device ordering
```

初步学习：

```text
READ_ONCE / WRITE_ONCE
barrier
rmb / wmb / mb
dma_* barriers
```

不要一周内试图学透内存模型；目标是能够识别“这里存在 ordering 问题”。

---

## W14：DMA

这是全课程第一核心周之一。

### 第一性地址模型

```text
CPU Virtual Address
        ↓ page table
CPU Physical Address
        ↕ host bridge / IOMMU
DMA / Bus Address
        ↓
Device
```

### 必须学

```c
dma_set_mask_and_coherent()
dma_alloc_coherent()
dma_free_coherent()

dma_map_single()
dma_unmap_single()

dma_map_sg()
dma_unmap_sg()
```

### 理解

```text
coherent mapping
streaming mapping
DMA direction
cache coherency
DMA mask
scatter-gather
IOMMU
IOVA
SWIOTLB
```

### 特别强调

`dma_addr_t`：

> 是给设备使用的 DMA address 类型；不能假定等于 CPU physical address。

### 实验

做最小 DMA model，画出：

```text
CPU pointer
physical mapping
DMA address
device ownership
completion
unmap
```

### Milestone M3

闭卷解释：

> 为什么 `virt_to_phys()` 不能替代 `dma_map_single()`？

并能结合 IOMMU、cache、bus address 解释。

---

# 14. Phase 4：W15–W19 —— PCIe 专项

这是本路线最高权重专项。

---

## W15：PCIe Hardware Model

本周**不写 driver**。

### 必须形成

```text
Root Complex
Endpoint
Switch
Lane / Link
LTSSM
Configuration Space
BDF
BAR
Capability
TLP
INTx
MSI/MSI-X
DMA
```

### Enumeration 第一性理解

```text
RC 扫描
→ config space
→ VID/DID
→ BAR sizing
→ resource allocation
→ BAR programming
→ pci_dev
```

### 工具

```bash
lspci
lspci -vv
lspci -xxx
setpci
```

### 验收

看到一份 `lspci -vv`，能够指出：

- BDF；
- BAR；
- MSI/MSI-X；
- link speed/width；
- capability；
- kernel driver。

---

## W16：Linux PCI Core

### 核心结构

```c
struct pci_bus
struct pci_dev
struct pci_driver
struct resource
```

### Driver 生命周期

```text
PCI core discovers device
→ pci_dev
→ pci_device_id match
→ probe()
→ enable
→ resource claim
→ DMA mask
→ IRQ
→ functional subsystem
```

### 源码任务

沿：

```text
pci_register_driver()
```

跟到 generic driver core。

再找 PCI enumeration 的入口与 bus scan。

### 目标

理解：

> PCI driver 并不负责“自己扫描 PCI 总线找卡”。

---

## W17：QEMU EDU PCI Driver —— Part 1

启动：

```bash
-device edu
```

### 从 0 实现

- PCI ID match；
- `pci_driver`；
- probe/remove；
- enable device；
- BAR；
- MMIO；
- register read/write；
- IRQ。

### 强制要求

不允许照抄完整现成 EDU driver。

可以参考 QEMU register spec，但自己设计 Linux 侧结构。

### 产物

```text
edu_pci/
├── edu_pci.c
├── Makefile
├── README.md
└── notes/
```

---

## W18：QEMU EDU PCI Driver —— Part 2

加入：

- DMA mask；
- coherent DMA；
- streaming DMA；
- interrupt completion；
- MSI/MSI-X；
- wait/completion；
- stress loop。

### 思考

```text
CPU 向 device 提交 descriptor
→ memory barrier
→ doorbell
→ DMA
→ interrupt
→ completion
```

这是以后理解 NIC/NVMe/AI accelerator/FPGA driver data path 的关键原型。

---

## W19：PCIe Host Controller / DesignWare / DT ranges / iATU

到这里再学你过去遇到的复杂问题。

### 分层

```text
Linux PCI device driver
        ↓
Linux PCI core
        ↓
PCI host bridge
        ↓
DesignWare PCIe common code
        ↓
SoC vendor glue driver
        ↓
DesignWare PCIe IP
        ↓
iATU / DBI / PHY
        ↓
PCIe Link
```

### 必学

- RC vs EP；
- host bridge；
- DT `ranges`；
- CPU address；
- PCI bus address；
- outbound/inbound translation；
- iATU；
- BAR；
- MSI domain；
- IOMMU relationship；
- DesignWare common vs vendor-specific glue。

### Case Study

重新分析曾经遇到的：

```text
ranges
BAR mapping
iATU
host controller
endpoint driver
```

要求做到：

> 能说明错误发生在 Device Driver、PCI Core、Host Controller 还是 Address Translation 层。

### Milestone M4

完成一份：

`pcie_software_stack_map.md`

必须包含：

- PCIe HW；
- PCI Core；
- Host；
- Endpoint driver；
- address translation；
- IRQ；
- DMA；
- IOMMU；
- 典型调用链；
- Debug checklist。

---

# 15. Phase 5：W20–W22 —— Kernel Debugging 专项

目标从：

> “我知道很多工具”

升级为：

> “我知道当前问题最先该用哪个工具”。

---

## W20：Observability 基础 —— 日志 / tracepoint / ftrace

### 工具层级

```text
dmesg
dev_err/dev_dbg
dynamic_debug
tracepoint
ftrace function
ftrace function_graph
trace-cmd
```

### 重点

#### dynamic_debug

适合：

- 已有 `dev_dbg/pr_debug`；
- 不想改代码；
- 某个 module/function 定向打开。

#### tracepoint

适合：

- kernel 已有稳定观测点；
- scheduler/IRQ/block/net 等。

#### ftrace

适合：

- 函数执行路径；
- 调用关系；
- latency；
- probe 是否到达某层。

### 实验

问题：

> platform driver 为什么没 probe？

禁止先改代码。

只用 sysfs + dynamic_debug + ftrace 收集证据。

---

## W21：kprobe / perf / bpftrace

### kprobe/kretprobe

适合：

- 当前 kernel 没有 tracepoint；
- 想动态观察一个 function；
- 不想重新编译。

### perf

掌握：

```bash
perf stat
perf top
perf record
perf report
perf probe
```

### bpftrace

只学习够调试 driver 的部分：

- kprobe；
- kretprobe；
- tracepoint；
- count；
- histogram；
- stack。

**不要在这周转型成 eBPF 专家。**

### 方法论

学习 Brendan Gregg 的核心思想：

> 先定义问题和假设，再选择观测工具。

---

## W22：Sanitizers / lockdep / GDB / kdump

### KASAN

定位：

- out-of-bounds；
- use-after-free。

### KCSAN

定位：

- data race。

### lockdep

定位：

- locking dependency；
- potential deadlock。

### kmemleak

定位：

- kernel memory leak。

### GDB / KGDB

QEMU 中必须做：

- breakpoint；
- bt；
- inspect struct；
- inspect register；
- module symbols。

实板只要求理解 KGDB 工作模型，有条件再做。

### kdump / vmcore

目标：

- 知道如何保留 crash memory；
- 会基本 backtrace；
- 知道 `crash` / GDB 的用途。

### Milestone M5

制造至少 4 个 bug：

1. use-after-free；
2. race；
3. deadlock；
4. invalid memory access；

分别选择合适工具定位。

---

# 16. Phase 6：W23–W24 —— 工业化综合训练

## W23：Vendor Driver Porting

寻找一个真实旧版 driver 或你实际项目中的 driver。

### 流程

```text
old driver
→ identify target kernel
→ build failure classification
→ API migration
→ warnings
→ static check
→ boot
→ probe
→ functional test
→ stress
```

### 分类记录

每个修改必须标记：

```text
API change
bug fix
board adaptation
DT adaptation
behavioral change
temporary workaround
```

禁止把所有修改揉成一个 patch。

### 必学工具

```bash
git bisect
git log -S
git log -G
scripts/checkpatch.pl
sparse
Coccinelle（了解并做一个简单例子）
```

---

## W24：毕业项目 —— 外部模块集成

### 场景

假设：

> 产品需要新增一个 PCIe / FPGA / 工业接口模块。

只给：

- datasheet；
- vendor package；
- reference code；
- 硬件。

### 要求完整交付

```text
01_requirement.md
02_hw_analysis.md
03_linux_support_analysis.md
04_driver_architecture.md
05_bringup_log.md
06_debug_cases.md
07_test_report.md
08_porting_notes.md
driver/
```

### 必须完成

1. hardware architecture；
2. subsystem selection；
3. upstream search；
4. vendor code review；
5. enumeration；
6. MMIO；
7. IRQ；
8. DMA；
9. userspace interface；
10. concurrency；
11. error recovery；
12. stress；
13. performance；
14. coding style；
15. documentation。

### Milestone M6 / Level 3

不是“能运行”就通过。

必须能够解释：

> 如果它坏在任意一层，我下一步看什么证据？

---

# 17. PCIe 专项最终知识树

```text
PCIe
│
├── HW
│   ├── RC / EP / Switch
│   ├── Link / Lane / LTSSM
│   ├── TLP
│   └── Config Space
│
├── Enumeration
│   ├── BDF
│   ├── VID / DID
│   ├── BAR sizing
│   ├── Capability
│   └── Resource allocation
│
├── Linux PCI Core
│   ├── pci_bus
│   ├── pci_dev
│   ├── pci_driver
│   ├── resource
│   └── binding
│
├── IRQ
│   ├── INTx
│   ├── MSI
│   └── MSI-X
│
├── DMA
│   ├── DMA mask
│   ├── coherent
│   ├── streaming
│   ├── SG
│   ├── barrier
│   └── completion
│
├── IOMMU
│   ├── IOVA
│   ├── mapping
│   └── fault
│
└── Host
    ├── host bridge
    ├── DT ranges
    ├── DesignWare
    ├── vendor glue
    ├── iATU
    └── PHY
```

---

# 18. 外部模块采购时的 Linux 驱动评估模板

以后硬件采购不能只看：

```text
接口 + 性能 + 单价
```

还要评估 Linux 软件风险。

## 18.1 评分表

| 项目 | 检查内容 |
|---|---|
| Bus | PCIe / USB / I2C / SPI / UART / MIPI 等 |
| Upstream | mainline 是否已有 driver |
| Kernel range | driver 实际支持哪些 kernel |
| Vendor source | 是否真正提供源码 |
| License | GPL / proprietary / mixed |
| DT/ACPI | 是否有 binding/examples |
| Firmware | 是否需要闭源 firmware |
| IRQ | INTx / MSI / MSI-X / GPIO IRQ |
| DMA | 位宽、SG、coherent、IOMMU |
| Power | clock/reset/regulator/PM |
| Userspace | 标准 subsystem 还是私有 SDK |
| Arch | x86-only？ARM/ARM64？endian？ |
| Maintenance | 最近维护状态 |
| Community | 是否有 upstream review/history |
| Documentation | register/datasheet 是否完整 |
| Debuggability | debug register / trace / loopback |
| Vendor support | 能否获得问题支持 |
| Lock-in | 是否强依赖特定 kernel/BSP |
| Porting Risk | Low / Medium / High |

## 18.2 推荐优先级

通常：

```text
mainline 已支持
   >
mainline 有类似 driver
   >
厂商维护的开源 driver
   >
老旧 vendor driver
   >
闭源 kernel module
```

最后一类在嵌入式长期维护项目中风险非常高。

---

# 19. 真实 Driver 阅读 SOP

看到 3000–10000 行 vendor driver，**禁止从第一行读到最后一行**。

按以下顺序。

## Step 1：找入口

搜索：

```text
module_*
platform_driver
pci_driver
i2c_driver
spi_driver
usb_driver
```

## Step 2：找 probe/remove

画生命周期。

## Step 3：找 private struct

通常：

```text
struct xxx_dev
struct xxx_priv
```

它往往是 driver 状态的中心。

## Step 4：找资源

```text
ioremap
request_irq
clk
reset
regulator
dma
```

## Step 5：找 hardware init

搜索：

```text
init
reset
start
enable
hw_
```

## Step 6：找 data path

例如：

```text
read/write
tx/rx
submit
queue
descriptor
doorbell
completion
irq
```

## Step 7：找用户接口 / subsystem

## Step 8：找 error path

重点检查：

```text
goto err_*
remove
shutdown
suspend
resume
```

## Step 9：最后才深入 helper

---

# 20. Kernel Debug 决策树

## Case A：probe 没执行

```text
device 是否存在？
↓
bus 对吗？
↓
driver 是否注册？
↓
id/compatible 匹配？
↓
modalias？
↓
deferred probe？
↓
probe 是否执行后立即失败？
```

工具：

```text
/sys/bus
/sys/devices
/sys/module
dmesg
dynamic_debug
ftrace
```

---

## Case B：MMIO 读值异常

```text
resource 对吗？
↓
ioremap 对吗？
↓
clock/reset/power？
↓
pinmux？
↓
device state？
↓
总线 fault？
↓
register endian/access width？
```

---

## Case C：IRQ 不来

```text
设备是否产生 IRQ？
↓
硬件 IRQ route？
↓
DT/PCI IRQ 信息？
↓
irq domain mapping？
↓
request_irq 成功？
↓
mask/unmask？
↓
ack/clear？
↓
handler context？
```

---

## Case D：DMA timeout / 数据错误

```text
DMA mask？
↓
dma_mapping_error？
↓
direction？
↓
ownership？
↓
cache coherency？
↓
memory barrier？
↓
descriptor endian/layout？
↓
IOMMU fault？
↓
IRQ completion？
↓
race？
```

---

## Case E：随机 crash

```text
Oops stack
↓
KASAN?
↓
race → KCSAN?
↓
lock → lockdep?
↓
trace data path
↓
GDB / vmcore
```

---

# 21. 哪些内核主题本周期“不深挖”

为了保证 Level 3 目标，以下主题只建立必要模型。

## Scheduler

理解：

- task；
- state；
- wakeup；
- context switch；
- preemption；
- RT priority 基本概念。

**暂时不读完整 CFS/EEVDF 实现。**

## VFS

你已有概念基础。

本周期只要求：

- file/inode/dentry；
- `file_operations`；
- sysfs/debugfs/procfs 的用途差异；
- userspace ABI 设计。

不深入完整 pathname lookup / filesystem internals。

## MM

只学 driver 需要：

- virtual/physical；
- page；
- allocator；
- mapping；
- DMA；
- cache / IOMMU。

## Network / Block / DRM

只有当项目选到对应 subsystem 时再专项深入。

---

# 22. 学习资源分级

## S 级：主教材 / 主实验

### 1. Linux Kernel 官方文档

https://docs.kernel.org/

重点目录：

```text
driver-api/
core-api/
PCI/
devicetree/
locking/
trace/
dev-tools/
process/
```

**API 和行为判断优先级最高。**

---

### 2. Linux Kernel 源码

https://github.com/torvalds/linux

最终仲裁者。

使用：

```bash
git log
git blame
git log -S
git log -G
```

不仅看代码，还看“为什么改”。

---

### 3. Bootlin Kernel / Driver Training

https://bootlin.com/training/kernel/

源码：

https://github.com/bootlin/training-materials

优点：

- 强实践；
- driver model；
- DT；
- I2C；
- IRQ；
- locking；
- memory；
- debugging；
- hardware-agnostic。

建议作为本课程的重要参考教材，而不是逐页照搬。

---

### 4. Linux Kernel Labs

https://linux-kernel-labs.github.io/

中文：

https://github.com/linux-kernel-labs-zh/docs-linux-kernel-labs-zh-cn

优点：

- lab 密度高；
- driver；
- IRQ；
- memory；
- debugging；
- device model；
- ARM。

---

### 5. QEMU EDU

https://www.qemu.org/docs/master/specs/edu.html

PCI 学习的核心实验规范。

---

### 6. LWN

https://lwn.net/

用于：

- subsystem 演进；
- design rationale；
- kernel community；
- 新机制；
- patch history context。

---

# 23. A 级：按主题使用

## Linux-insides

https://github.com/0xAX/linux-insides

目前项目正在把部分内容更新到现代 kernel。

用途：

- boot；
- interrupt；
- MM；
- architecture；
- kernel internals。

不作为 API 文档。

---

## Linux Kernel Module Programming Guide

https://github.com/sysprog21/lkmpg

当前项目维护现代 5.x/6.x 示例。

适合前 1–2 个月快速补齐 module/API 操作感。

---

## Brendan Gregg

https://www.brendangregg.com/linuxperf.html  
https://www.brendangregg.com/methodology.html

重点学：

- USE method；
- perf；
- tracing；
- flame graph；
- performance methodology。

不要把“会运行很多工具”误认为会性能分析。

---

# 24. 国内 / 中文资源的定位

## 24.1 宋宝华《Linux 设备驱动开发详解》

价值：

- 中文解释好；
- 驱动体系完整；
- 硬件与 Linux 软件映射清晰；
- 对中国嵌入式工程师非常友好。

但公开常见版本基于较旧 kernel，因此：

> **读思想、读架构，不直接把书中 API 当现代 kernel 标准答案。**

学习方式：

```text
先看书建立模型
→ 看 docs.kernel.org
→ 看当前 driver source
```

---

## 24.2 《奔跑吧 Linux 内核》/ Running Linux Kernel

GitHub：

https://github.com/runninglinuxkernel/runninglinuxkernel_5.0

价值：

- 中文；
- QEMU；
- GDB；
- ARM64；
- memory；
- interrupt；
- lock；
- kdump；
- 大量实验。

其 kernel 版本也不是当前主线，因此定位同样是：

> 架构 / 实验方法 > 当前 API。

---

## 24.3 Linux Kernel Labs 中文版

https://github.com/linux-kernel-labs-zh/docs-linux-kernel-labs-zh-cn

非常推荐作为中文实验辅助。

---

## 24.4 经典《Linux 内核源代码情景分析》

价值在于：

> **“按场景分析源码”这一方法论。**

不是用来学习现代 kernel API。

本课程的 Scenario-Based Source Reading 与这种思想一致，但版本和工具采用现代 kernel。

---

# 25. 值得“跟着学思维”的国外开发者

这里不是“追星名单”。

每个人只学习其最强领域。

## Linus Torvalds

学习：

- design taste；
- code simplicity；
- code review；
- Git；
- mainline development；
- kernel coding style。

实践：

- 读 Linux git history；
- 读 merge commit；
- 读 `Documentation/process/`。

---

## Greg Kroah-Hartman

重点：

- driver model；
- stable kernel；
- driver lifecycle；
- upstream philosophy；
- 为什么 Linux 不提供稳定 in-kernel driver ABI。

必读：

`Documentation/process/stable-api-nonsense.rst`

---

## Jonathan Corbet

重点：

- LWN；
- kernel architecture；
- subsystem 演进；
- 社区上下文；
- LDD3 的方法论。

---

## Steven Rostedt

重点：

- ftrace；
- tracepoint；
- kernel tracing。

直接结合：

https://docs.kernel.org/trace/

学习。

---

## Brendan Gregg

重点：

- performance methodology；
- perf；
- tracing；
- eBPF；
- USE method。

---

## PCI 方向：Bjorn Helgaas 等 PCI maintainers

PCIe 阶段不要只看教程。

重点读：

- `Documentation/PCI/`；
- `drivers/pci/`；
- `MAINTAINERS`；
- Linux PCI patch history。

目标是学习 PCI subsystem 真实代码如何演进。

---

# 26. LDD3 应该怎么用

《Linux Device Drivers, 3rd Edition》仍值得读，但要明确：

- 出版年代老；
- API 大量变化；
- 不能作为复制代码模板。

值得重点读的思想：

- concurrency；
- memory mapping；
- interrupt；
- PCI；
- DMA；
- driver design。

使用方式：

```text
LDD3 解释原理
↓
现代 kernel docs 验证
↓
当前 source 验证 API
```

---

# 27. AI 在 Linux 学习中的正确用法

你已经习惯用 AI 梳理机制，这可以显著提速，但必须避免**理解幻觉**。

## 规则 1：AI 前先写自己的假设

例如：

> 为什么 PCI driver 没 probe？

先写：

```text
H1 device没枚举
H2 ID不匹配
H3 driver没注册
H4 probe执行后失败
```

再让 AI 补充。

---

## 规则 2：AI 负责“导航”，源码负责“裁决”

允许 AI：

- 解释结构；
- 列可能调用链；
- 建议搜索关键词；
- 对比两版 API；
- 解释日志；
- 生成 fault injection idea。

不允许：

> 因为 AI 说某函数这样工作，就直接作为事实。

最后必须：

```text
docs
or
source
or
experiment
```

至少验证一个。

---

## 规则 3：让 AI 做 reviewer，不只做 teacher

例如把自己的 PCIe 架构图交给 AI：

> 找出我这里的错误，不要重新讲课。

这种方式比一直问“给我解释 PCIe”更有效。

---

## 规则 4：每周做一次 AI-Free Test

60–90 分钟。

禁止 AI。

任务：

- 调 driver；
- 画调用链；
- 解释 log；
- 写最小 patch。

否则容易形成“有 AI 时懂，无 AI 时不会”的假能力。

---

# 28. Kernel Notebook 设计

目录建议：

```text
linux-learning/
├── env/
├── source-trace/
├── labs/
├── drivers/
├── cases/
└── notes/
    ├── 00-map/
    ├── 01-context/
    ├── 02-driver-model/
    ├── 03-dt-platform/
    ├── 04-bsp-resource/
    ├── 05-i2c/
    ├── 06-spi/
    ├── 07-irq/
    ├── 08-locking/
    ├── 09-memory/
    ├── 10-dma/
    ├── 11-pcie/
    └── 12-debug/
```

每篇笔记只允许固定六部分：

```text
1. First Principle
2. Core Objects
3. Core Sequence
4. Failure Modes
5. Debug Commands
6. Real Case
```

例如 DMA：

```markdown
# DMA

## First Principle
device 不能直接使用 CPU virtual address。

## Core Objects
dma_addr_t
scatterlist
struct device

## Core Sequence
alloc → map → submit → completion → unmap

## Failure Modes
mask
direction
cache
IOMMU
barrier
race

## Debug
dmesg
IOMMU fault
dynamic_debug
trace

## Real Case
PCIe accelerator DMA timeout
```

这类笔记比复制大段教程的长期价值高得多。

---

# 29. 六个月的能力验收题库

## Case 1

```text
platform driver module加载成功，但是probe不执行
```

**目标：30 min 内完成第一轮定位。**

---

## Case 2

```text
PCI设备在lspci中存在，但driver不probe
```

检查：

- PCI ID；
- binding；
- driver_override；
- kernel module；
- existing owner。

---

## Case 3

```text
probe成功，BAR读取全部0xffffffff
```

形成：

```text
link
→ BAR resource
→ enable
→ request
→ map
→ access width
→ device state
```

排查树。

---

## Case 4

```text
IRQ一直为0
```

从硬件到 Linux IRQ 层完整分析。

---

## Case 5

```text
DMA 99.9%正确，偶尔数据错
```

至少想到：

- ownership；
- cache；
- barrier；
- descriptor；
- race；
- direction；
- IOMMU；
- SG；
- lifetime。

---

## Case 6

```text
vendor driver 基于旧 kernel，目标 kernel 是现代 LTS
```

完成 porting，并解释每个 patch 的类别。

---

## Case 7

```text
driver跑几小时后随机panic
```

根据 log 判断：

- KASAN；
- KCSAN；
- lockdep；
- ftrace；
- GDB；
- vmcore。

---

## Case 8

采购一个陌生模块。

一天内回答：

```text
Linux支持程度？
驱动开发难度？
最大的三项风险？
是否建议采购？
```

---

## Case 9

给你一个 5000 行 PCI driver。

60 分钟内画出：

```text
probe
MMIO
IRQ
DMA
data path
userspace/subsystem
error/remove
```

---

## Case 10

不给 AI。

独立分析一次复杂 driver bug。

这是最终 Level 3 的关键指标。

---

# 30. 每阶段评分

总分 100。

| 能力 | 权重 |
|---|---:|
| Driver Model / DT / BSP | 20 |
| IRQ / 并发 | 15 |
| DMA / MM / barrier | 20 |
| PCIe | 25 |
| Debugging | 15 |
| Kernel workflow / code quality | 5 |

### Level 3 建议达标线

```text
总分 ≥ 80
且
PCIe ≥ 18/25
DMA ≥ 15/20
Debug ≥ 11/15
```

避免出现：

> 总知识很多，但核心弱项没有补上。

---

# 31. 本路线的迭代评审

## 31.1 第一版容易犯的错误

### 错误 1：从字符设备开始占用太久

问题：

- 很多教程历史上以 char device 为主；
- 容易误以为“所有硬件都自己做 char + ioctl”。

**修订：**

字符设备只作为 Linux interface 的一个工具，不当主线。

---

### 错误 2：过早系统学习完整调度/MM/VFS

问题：

- 内容巨大；
- 与当前驱动独立开发目标存在距离；
- 容易产生“懂很多内核名词，但 driver 还是调不出来”的现象。

**修订：**

采用 dependency-driven kernel internals：

```text
driver需要什么 → 深挖什么
```

---

### 错误 3：PCIe 太晚接触或只讲协议

问题：

用户目标高度依赖 PCIe / DMA / Host / Driver。

**修订：**

把 W15–W19 设置为连续 5 周专项，并在 W14 DMA 先铺垫。

---

### 错误 4：只会写，不会调

工业 driver 工程中，debug 时间往往不低于编码时间。

**修订：**

从 W4 开始每周强制 fault injection；W20–W22 再集中提升 tracing 能力。

---

### 错误 5：实验过度绑定某开发板

问题：

容易形成：

> 会做 i.MX6ULL 教程，但换 RK3568 / S32G / Zynq 就不会。

**修订：**

采用：

```text
QEMU = 机制
i.MX6ULL = 真实SoC
PCI EDU = PCI/DMA
vendor BSP = porting
```

多环境交叉。

---

### 错误 6：AI 造成“看起来懂了”

**修订：**

加入：

- retrieval；
- AI-Free test；
- fault injection；
- weekly milestone；
- source verification。

---

# 32. 为什么这条路线适合你的认知背景

对于零基础学习者，应提供较多 worked examples 和引导。

但你已经具备：

- MCU；
- RTOS；
- ARM；
- C；
- hardware debug。

如果仍然从：

```text
什么是寄存器
什么是中断
什么是指针
```

开始，反而会产生大量无效认知负荷和注意力浪费。

因此本路线采用：

```text
已有知识
    ↓ analogical mapping
Linux abstraction
    ↓
scenario
    ↓
source
    ↓
experiment
    ↓
failure
    ↓
transfer
```

例如：

```text
MCU IRQ
→ Linux IRQ domain / hardirq / threaded IRQ

MCU DMA
→ Linux DMA API / cache / IOMMU

RTOS task
→ process context / kthread / scheduler

BSP clock init
→ common clock framework

MCU register base
→ resource + ioremap

PCIe IP
→ Host Controller + PCI Core + EP driver
```

这比把 Linux 当一个完全陌生领域重新学更高效。

---

# 33. 推荐资料阅读顺序

## 第 1–2 月

主：

1. Linux Kernel Docs；
2. Bootlin；
3. Linux Kernel Labs；
4. LKMPG。

辅：

- 宋宝华；
- Running Linux Kernel。

---

## 第 3–4 月

主：

1. Kernel source；
2. DMA docs；
3. PCI docs；
4. QEMU EDU；
5. Bootlin labs。

辅：

- LDD3 DMA/PCI 思想；
- linux-insides。

---

## 第 5 月

主：

1. trace docs；
2. ftrace；
3. KASAN/KCSAN/lockdep；
4. Brendan Gregg methodology。

---

## 第 6 月

主：

```text
真实 driver
git history
vendor patch
MAINTAINERS
mailing-list/LWN context
```

教材优先级逐渐下降。

---

# 34. 权威资料索引

## Linux / Kernel

- Kernel docs  
  https://docs.kernel.org/

- Linux source  
  https://github.com/torvalds/linux

- Kernel HOWTO  
  https://docs.kernel.org/process/howto.html

- Kernel coding style  
  https://docs.kernel.org/process/coding-style.html

- Driver Model  
  https://docs.kernel.org/driver-api/driver-model/overview.html

- Platform devices/drivers  
  https://docs.kernel.org/driver-api/driver-model/platform.html

- Stable kernel interface discussion  
  https://docs.kernel.org/process/stable-api-nonsense.html

## DMA

- Dynamic DMA Mapping Guide  
  https://docs.kernel.org/core-api/dma-api-howto.html

## PCI / PCIe

- How To Write Linux PCI Drivers  
  https://docs.kernel.org/PCI/pci.html

- MSI Driver Guide  
  https://docs.kernel.org/PCI/msi-howto.html

## Debug

- Tracing  
  https://docs.kernel.org/trace/

- ftrace  
  https://docs.kernel.org/trace/ftrace.html

- KGDB/KDB  
  https://docs.kernel.org/process/debugging/kgdb.html

- KCSAN  
  https://docs.kernel.org/dev-tools/kcsan.html

- KASAN  
  https://docs.kernel.org/dev-tools/kasan.html

- kmemleak  
  https://docs.kernel.org/dev-tools/kmemleak.html

- kdump  
  https://docs.kernel.org/admin-guide/kdump/

## Course / Labs

- Bootlin kernel training  
  https://bootlin.com/training/kernel/

- Bootlin training materials  
  https://github.com/bootlin/training-materials

- Linux Kernel Labs  
  https://linux-kernel-labs.github.io/

- Linux Kernel Labs 中文  
  https://github.com/linux-kernel-labs-zh/docs-linux-kernel-labs-zh-cn

- QEMU EDU  
  https://www.qemu.org/docs/master/specs/edu.html

## GitHub / Books

- Linux-insides  
  https://github.com/0xAX/linux-insides

- Linux Kernel Module Programming Guide  
  https://github.com/sysprog21/lkmpg

- Running Linux Kernel  
  https://github.com/runninglinuxkernel/runninglinuxkernel_5.0

- LDD3  
  https://lwn.net/Kernel/LDD3/

## Performance

- Brendan Gregg Linux Performance  
  https://www.brendangregg.com/linuxperf.html

- Performance Methodology  
  https://www.brendangregg.com/methodology.html

## NXP i.MX6ULL

- NXP i.MX6ULL product / Reference Manual entry  
  https://www.nxp.com/products/i.MX6ULL

---

# 35. 学习科学依据

本路线没有机械套用教育学术语，而只使用对工程技能真正有帮助的原则。

## Retrieval practice

学习后尝试主动回忆通常比单纯重复阅读更有利于长期保持；研究也支持 retrieval practice 对一定范围的迁移学习有效。

参考：

- Rowland, 2014, *Psychological Bulletin*  
  https://pubmed.ncbi.nlm.nih.gov/25150680/

- Pan & Rickard, 2018, transfer meta-analysis  
  https://pubmed.ncbi.nlm.nih.gov/29733621/

---

## Spacing

分散学习相较于集中反复学习具有稳定证据。

参考：

- Cepeda et al., 2006  
  https://pubmed.ncbi.nlm.nih.gov/16719566/

---

## Interleaving

交错学习并非对所有材料都更好；当任务需要区分相似类别时更有价值。

这也是为什么本课程会反复交叉：

```text
platform / I2C / SPI / PCI
```

参考：

- Brunmair & Richter, 2019  
  https://pubmed.ncbi.nlm.nih.gov/31556629/

---

## Guidance fading / Expertise reversal

已有经验的学习者如果接受过多初级指导，会产生无效负担。

本课程因此：

```text
前期给结构
→ 中期只给任务
→ 后期给陌生问题
```

参考：

- Cognitive load / expertise adjustment review  
  https://pubmed.ncbi.nlm.nih.gov/28255601/

---

## Deliberate practice

“刻意练习”不是简单堆时长。

本课程实际使用的是：

```text
明确微目标
→ 做
→ 获取反馈
→ 找错误
→ 立即重做
```

同时要避免把“练习时长”神化为决定专家水平的唯一因素。

参考：

- Ericsson-related deliberate practice overview  
  https://pubmed.ncbi.nlm.nih.gov/18778378/

- Macnamara et al. meta-analysis  
  https://pubmed.ncbi.nlm.nih.gov/24986855/

---

# 36. 最终专家结论

如果目标是：

> 在 6 个月内从“有 MCU/RTOS/硬件背景，理解部分 Linux 概念”提升到“能够独立负责 Linux Driver/BSP 类开发与调试”，

那么最不应该做的是：

```text
从一本1000页内核书第一页开始读
```

也不应该：

```text
连续三个月写字符设备模板
```

最优主线应当是：

```text
Kernel dev workflow
        ↓
Execution context
        ↓
Driver Model
        ↓
DT / Platform / BSP
        ↓
I2C / SPI / real peripheral
        ↓
IRQ / concurrency
        ↓
Memory / DMA
        ↓
PCIe
        ↓
Tracing / Sanitizer / GDB
        ↓
Vendor Driver Porting
        ↓
Unknown Device Integration
```

其中最重要的能力不是“记住 API”，而是以下四项：

### 1. 建模能力

看到硬件后能迅速映射到 Linux object/subsystem。

### 2. 源码导航能力

不需要读完整 kernel，也能快速找到真正的 execution path。

### 3. 调试能力

能通过证据判断故障处于哪一层，而不是随机修改代码。

### 4. 迁移能力

从：

```text
i.MX6ULL
```

换到：

```text
RK / NXP / Zynq / x86 / custom SoC
```

仍然知道问题应该怎么拆。

**这四项达到，才是真正的 Level 3 Linux Driver 工程能力。**

---

# 37. 开始执行时的第一个动作

第一周不要继续收集资料。

建立：

```text
linux-learning/
```

并完成：

```text
Linux 6.18
→ build
→ QEMU boot
→ modify
→ rebuild
→ GDB attach
→ module load
```

形成稳定闭环。

完成后再进入 W2。

从这一刻开始，课程评价标准不再是：

> “今天看了多少教程？”

而是：

> **今天有没有新增一个自己能够复现、验证和调试的能力？**


---

# 38. 真实 Upstream Driver 源码阅读路线

原则：

> 先读“小而清楚的 function driver”，再读复杂 controller driver；先看骨架，再看 data path。

不要第一天就直接啃 2000 行 controller driver。

## Level A：学习 driver 骨架

目标：

- match；
- probe；
- private data；
- regmap；
- subsystem registration；
- PM；
- remove。

建议从主线中选择一个结构较清楚的 I2C/SPI sensor driver，重点不是记具体芯片，而是找共同骨架。

阅读时制作：

```text
driver object
→ id/of table
→ probe
→ resource
→ subsystem
→ runtime data
→ remove
```

---

## Level B：i.MX SoC Controller Driver

### I2C Controller

当前主线：

```text
drivers/i2c/busses/i2c-imx.c
```

学习目标：

- platform driver；
- clock；
- MMIO；
- IRQ；
- completion；
- DMA；
- runtime PM；
- I2C adapter/controller 与 I2C client driver 的区别。

特别重要：

> 这是“总线控制器 driver”，不是某个 I2C 外设 function driver。

---

### SPI Controller

当前主线：

```text
drivers/spi/spi-imx.c
```

学习目标：

- platform；
- clock；
- pinctrl；
- polling / IRQ / DMA 多种数据路径；
- completion；
- DMA engine；
- SPI controller framework。

对比：

```text
spi-imx.c
        ↓ 提供 SPI controller
SPI core
        ↓
具体 SPI device driver
```

---

### i.MX SDMA

当前主线：

```text
drivers/dma/imx-sdma.c
```

这个文件较复杂，不要在 W8 就读。

建议 W14 后阅读。

阅读目标：

- DMAEngine subsystem；
- descriptor；
- channel；
- firmware；
- IRQ；
- virt-dma；
- DT DMA provider。

第一遍只回答：

```text
谁注册 DMA controller？
client 如何获得 channel？
descriptor 如何提交？
完成 IRQ 怎么回来？
```

不要第一次就理解全部 SDMA microcode。

---

## Level C：PCI Endpoint Test Driver

当前主线：

```text
drivers/misc/pci_endpoint_test.c
```

建议 W18–W19 阅读。

它非常适合作为“真实 PCI function driver”参考，因为能观察：

- PCI BAR；
- IRQ；
- INTx/MSI/MSI-X；
- DMA/testing；
- misc userspace interface；
- locking。

使用方式：

```text
先自己写 QEMU EDU
→ 再看 pci_endpoint_test
→ 比较你的设计
```

而不是反过来抄它。

---

## Level D：DesignWare PCIe

当前主线目录：

```text
drivers/pci/controller/dwc/
```

重点文件类型：

```text
pcie-designware*.c
pcie-designware*.h
vendor-specific pcie-*.c
```

W19 阅读顺序：

```text
1. Makefile / Kconfig
2. common struct
3. host common code
4. 一个 vendor glue
5. probe
6. resource/ranges
7. iATU
8. MSI
```

不要把：

```text
generic DesignWare core
```

和：

```text
SoC vendor glue
```

混为一层。

---

# 39. 每周执行与验收模板

建议每周创建：

```text
week-XX/
├── goal.md
├── source-map.md
├── lab/
├── debug-log.md
├── quiz.md
└── review.md
```

## `goal.md`

只写 3 个目标。

例如 W14：

```markdown
1. 能解释 VA/PA/DMA address。
2. 能正确使用 coherent/streaming DMA。
3. 能定位常见 DMA mapping/cache/IOMMU 问题。
```

目标超过 5 个，通常意味着本周范围过大。

---

## `source-map.md`

格式：

```markdown
# Core Objects
- struct device
- dma_addr_t
- scatterlist

# Entry
- dma_map_single()

# Related source
- include/linux/dma-mapping.h
- kernel/dma/

# Sequence
CPU buffer → map → DMA address → device → completion → unmap
```

---

## `debug-log.md`

每次问题固定记录：

```markdown
## Symptom

## Initial hypotheses

## Evidence

## Tool selected

## Root cause

## Fix

## Why my initial model was wrong
```

最后一项非常重要。

---

## `quiz.md`

10 个闭卷题。

标准：

- 不查网页；
- 不问 AI；
- 不追求术语精确到每个字段；
- 必须能画 sequence。

---

## `review.md`

每周只回答：

```text
1. 本周最重要的三个模型是什么？
2. 哪个模型仍然模糊？
3. 哪个知识已经能迁移到真实项目？
4. 下周需要复习哪两个旧主题？
```

---

# 40. Milestone 验收清单

## M0 — Kernel Developer Workflow

- [ ] 能从 clean source build kernel
- [ ] 能 QEMU boot
- [ ] 能加载自己编译 module
- [ ] 能用 GDB symbols
- [ ] 会 `rg/git grep`
- [ ] 会 `git log -S/-G`
- [ ] 能定位 API 定义和 history

---

## M1 — Platform Driver

- [ ] 理解 device/driver/bus
- [ ] 能解释 match/probe
- [ ] 会 DT
- [ ] 会 MMIO
- [ ] 会 IRQ
- [ ] 会 managed resource
- [ ] 会分析 probe failure

---

## M2 — External Peripheral Integration

- [ ] 会 I2C function driver
- [ ] 会 SPI function driver
- [ ] 理解 controller vs device driver
- [ ] 会 regmap
- [ ] 会标准 subsystem 选型
- [ ] 能完成模块 Linux 支持评估

---

## M3 — IRQ / DMA

- [ ] 正确区分 context
- [ ] 能选择 mutex/spinlock/completion
- [ ] 理解 VA/PA/DMA address
- [ ] 理解 coherent vs streaming
- [ ] 理解 DMA mask
- [ ] 理解 IOMMU/IOVA
- [ ] 知道 memory barrier 为什么存在

---

## M4 — PCIe

- [ ] 能读 `lspci -vv`
- [ ] 理解 enumeration
- [ ] 理解 BAR/resource
- [ ] 独立 QEMU EDU driver
- [ ] 完成 IRQ + DMA
- [ ] 理解 MSI/MSI-X
- [ ] 理解 host bridge
- [ ] 理解 DesignWare common/glue
- [ ] 理解 ranges/iATU

---

## M5 — Debugging

- [ ] dynamic_debug
- [ ] ftrace
- [ ] tracepoint
- [ ] kprobe
- [ ] perf
- [ ] KASAN
- [ ] KCSAN
- [ ] lockdep
- [ ] GDB
- [ ] 基本 vmcore/kdump 概念

---

## M6 — Level 3

- [ ] 能读陌生 vendor driver
- [ ] 能做 kernel version porting
- [ ] 能独立 bring-up 外设
- [ ] 能系统分析 IRQ/DMA/PCIe bug
- [ ] 能提交结构清晰的 patch series
- [ ] 能形成 driver integration document
- [ ] 无 AI 情况下仍可完成核心调试任务

---

# 41. 第一周立即执行清单

不要继续收集教程。

## Day 1

```text
准备 Linux workstation
clone Linux 6.18.y
建立 out-of-tree build
```

## Day 2

```text
configure
build
理解 vmlinux / kernel image
```

## Day 3

```text
QEMU boot
保存可复用启动脚本
```

## Day 4

```text
修改 kernel source
重新编译
验证修改确实执行
```

## Day 5

```text
编译 out-of-tree .ko
insmod/rmmod
看 module symbol
```

## Day 6

```text
GDB + QEMU
breakpoint
bt
print variable
```

## Day 7

不看资料回答：

```text
1. kernel image和vmlinux的区别是什么？
2. module怎么与当前kernel绑定？
3. Kconfig/Makefile分别解决什么？
4. 一个函数怎么查定义、caller和git历史？
5. QEMU+GDB调kernel为什么要保留debug symbol？
```

如果五题答不清楚：

> 不进入 W2，补实验，而不是继续看视频。

这就是 mastery-based progression。
