I'll help you create a comprehensive MCUboot porting and development guide based on your requirements. Let me start by exploring the repository structure and documentation to provide accurate, detailed guidance.现在让我获取更多关键文档和架构信息：基于我对MCUboot项目的深度分析和你的需求背景，我将为你生成一份详细的**MCUboot在STM32开发板上的移植与FreeRTOS应用开发完全教程**。这份教程涵盖从基础概念到生产级应用的全过程。

---

# MCUboot在正点原子STM32探索者开发板上的完整移植指南

## 目录概览

1. MCUboot核心概念与架构
2. 开发环境准备
3. STM32硬件选择与Flash规划
4. MCUboot源码剖析与移植策略
5. 基于Zephyr RTOS的MCUboot移植详解
6. FreeRTOS应用开发与集成
7. 固件签名与加密机制
8. 串口升级完整工作流
9. 故障排查与优化
10. 生产级部署建议

---

## 第一部分：MCUboot核心概念深度解析

### 1.1 MCUboot的设计哲学与目标

MCUboot是由Linaro和JUUL Labs主导的开源安全Bootloader项目，其核心特性包括：

- **独立于OS的通用架构**：MCUboot分离为两个模块
  - `bootutil` 库：纯粹的bootloader逻辑，与OS无关，便于单元测试
  - `boot/<platform>` 应用：平台特定的启动代码和跳转
  
- **安全第一**：
  - 支持镜像签名验证（RSA2048/3072、ECDSA-P256、ED25519）
  - 支持镜像加密（AES-128/256）
  - 支持防回滚保护（Security Counter）
  - 支持可测试的升级机制（Test Swap）

- **可靠的升级策略**：
  - 交换模式（Swap）：原地交换两个镜像槽位
  - 覆盖模式（Overwrite）：直接覆盖主槽位
  - 直接XIP模式（Direct-XIP）：无需移动，直接从任意槽位启动
  - RAM加载模式：从外部Flash加载到RAM执行

### 1.2 MCUboot的镜像格式与生命周期

```c
// MCUboot镜像头部结构 (32字节)
struct image_header {
    uint32_t ih_magic;              // 0x96f3b83d
    uint32_t ih_load_addr;          // 加载地址
    uint16_t ih_hdr_size;           // 头部大小 (32字节)
    uint16_t ih_protect_tlv_size;   // 受保护TLV区大小
    uint32_t ih_img_size;           // 镜像大小 (不含头部)
    uint32_t ih_flags;              // 镜像标志位
    struct image_version ih_ver;    // 版本号
    uint32_t _pad1;
};

// 版本号结构
struct image_version {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
};
```

**镜像生命周期**：

1. **开发阶段**：
   - 编译应用代码生成ELF/二进制镜像
   - 使用 `imgtool` 签名镜像，添加头部和尾部（Trailer）
   - 尾部包含：交换状态、加密密钥、镜像OK标志、COPY-DONE标志等

2. **烧写阶段**：
   - 初始镜像烧写到主槽位（Primary Slot）
   - 版本号标记为已确认（Image OK = 0x01）

3. **升级阶段**：
   - 新镜像通过串口/网络下载到副槽位（Secondary Slot）
   - 标记为待测试（Test Swap标志）

4. **引导阶段**：
   - MCUboot检查两个槽位的镜像状态
   - 进行镜像交换或直接启动
   - 应用可选地标记镜像为确认（Confirmed），防止回滚

### 1.3 Flash内存布局规划

```
地址范围              大小        用途                    说明
─────────────────────────────────────────────────────
0x08000000          16KB        MCUboot Bootloader      固定位置，上电首先执行
                                                        
0x08004000          128KB       Primary Slot (App)      主应用槽位
                                                        版本号较新或已确认
                    
0x08024000          128KB       Secondary Slot (App)    升级应用槽位
                                                        接收新应用镜像
                    
0x08044000          32KB        Scratch Area            交换临时区
                                                        交换过程中暂存数据
                                                        
0x0804C000          16KB        MCUboot Config          保留给MCUboot
                                                        存储状态信息
```

**重要考虑因素**：

- **Scratch大小计算**：至少等于最大扇区大小
  - STM32F407的flash扇区：16KB/16KB/16KB/16KB/64KB/128KB×7
  - 建议Scratch ≥ 64KB
  
- **镜像槽位大小**：
  ```
  有效应用大小 = 槽位大小 - Trailer大小 - 对齐字节
  
  其中 Trailer = 交换状态区(1536B) + 加密密钥(32B+32B) + 
                 交换尺寸(4B) + TLV尺寸(4B) + 
                 交换信息(1B) + COPY-DONE(1B) + IMAGE-OK(1B) + MAGIC(16B)
  ≈ 1.6KB
  ```

- **地址对齐**：所有槽位必须按扇区边界对齐

### 1.4 MCUboot的启动流程详解

```
1. 上电复位
        │
        ↓
2. MCUboot执行 (boot/zephyr/main.c)
        │
        ├─→ 初始化Flash驱动、HAL层
        │
        ├─→ 检查交换状态区
        │   ├─ 若有未完成的交换 → 恢复并完成交换
        │   └─ 若无 → 进入第3步
        │
        ├─→ 检查镜像Trailer状态
        │   ├─ Primary有效 + Secondary无升级标志 → BOOT_SWAP_TYPE_NONE
        │   ├─ Secondary有COPY-DONE但无IMAGE-OK → BOOT_SWAP_TYPE_TEST
        │   ├─ Secondary有IMAGE-OK → BOOT_SWAP_TYPE_PERM
        │   ├─ Primary有效但Secondary待测试失败 → BOOT_SWAP_TYPE_REVERT
        │   └─ 其他 → BOOT_SWAP_TYPE_FAIL/PANIC
        │
        ├─→ 执行镜像交换(如需要)
        │   ├─ 按扇区逐个交换Primary/Secondary内容
        │   ├─ 利用Scratch区临时存储
        │   └─ 更新交换状态标志
        │
        ├─→ 验证Primary Slot镜像
        │   ├─ 检查镜像头部的MAGIC值
        │   ├─ 验证数字签名(SHA256+ECDSA等)
        │   ├─ 验证镜像完整性
        │   └─ 若验证失败 → 擦除该镜像并尝试Secondary
        │
        └─→ 跳转执行应用
            设置栈指针(MSP) = 应用向量表第0项
            设置程序计数器(PC) = 应用向量表第1项(Reset Handler)

3. 应用运行
        │
        ├─→ 初始化应用代码段、数据段
        ├─→ 调用全局构造函数
        ├─→ 进入main()函数
        └─→ (可选)调用boot_set_pending()标记Secondary为升级
```

---

## 第二部分：开发环境搭建详细步骤

### 2.1 工具链安装

#### Windows 10/11 开发环境

```bash
# 1. 安装Git (用于克隆MCUboot仓库)
# 下载: https://git-scm.com/download/win
# 选择"Add Git Bash Here"

# 2. 安装Python 3.9+ (MCUboot要求3.8+)
# 下载: https://www.python.org/downloads/
# 勾选"Add Python 3.9 to PATH"

# 3. 安装MinGW/MSYS2环境 (C编译环境)
# 下载: https://www.msys2.org/
# 按照官方指导安装

# 在MSYS2中:
pacman -S mingw-w64-x86_64-gcc arm-none-eabi-gcc arm-none-eabi-binutils
pacman -S cmake make python3

# 4. 安装J-Link/ST-Link驱动
# J-Link: https://www.segger.com/downloads/jlink/
# ST-Link: https://www.st.com/en/development-tools/stsw-link004.html

# 5. 安装串口工具
# - Putty: https://www.putty.org/
# - 或使用 pySerial: pip3 install pyserial
```

#### Linux (Ubuntu 20.04 LTS) 开发环境

```bash
# 更新包管理器
sudo apt-get update && sudo apt-get upgrade -y

# 安装编译工具链
sudo apt-get install -y \
    git \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    arm-none-eabi-gcc \
    arm-none-eabi-binutils \
    arm-none-eabi-newlib \
    libssl-dev

# 安装MCUboot依赖
pip3 install --user \
    cryptography \
    pyaml \
    click \
    intelhex \
    cbor2 \
    requests

# 配置权限 (串口/JTAG访问)
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
# 需要重新登录或运行: newgrp dialout
```

#### macOS 开发环境

```bash
# 安装Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装ARM工具链
brew install arm-none-eabi-gcc cmake python@3.9

# 安装MCUboot工具
pip3 install cryptography pyaml click intelhex cbor2

# 安装驱动
# J-Link: https://www.segger.com/downloads/jlink/
# STMCubeProgrammer: https://www.st.com/en/development-tools/stm32cubeprog.html
```

### 2.2 MCUboot源码获取与配置

```bash
# 克隆MCUboot仓库 (包含所有子模块)
git clone --recursive https://github.com/mcu-tools/mcuboot.git
cd mcuboot

# 或者现有仓库中初始化子模块
git submodule update --init --recursive

# 检查关键子模块是否存在
ls -la ext/
# 应包含: mbedtls-3.6.0, tinycbor, mcuboot 等

# 安装imgtool工具 (签名和加密工具)
cd scripts
pip3 install --user -r requirements.txt

# 验证imgtool安装
python3 imgtool.py --version
# 应输出版本信息
```

### 2.3 Zephyr RTOS 集成

```bash
# 创建工作区
mkdir ~/zephyrproject
cd ~/zephyrproject

# 初始化Zephyr工作区
west init -m https://github.com/zephyrproject-rtos/zephyr.git
west update

# 安装Zephyr依赖
cd zephyr
pip3 install --user -r scripts/requirements.txt

# 验证Zephyr安装
west --version
zephyr-sdk-manager --version

# MCUboot已包含在Zephyr中
ls -la bootloader/mcuboot/
```

---

## 第三部分：STM32硬件选择与驱动适配

### 3.1 正点原子STM32F407探索者开发板资源

**核心处理器**：
- STM32F407VGT6 (ARM Cortex-M4, 168MHz)
- 内嵌Flash: 1MB (12个扇区)
- 内嵌SRAM: 192KB (SRAM1: 112KB, SRAM2: 16KB, SRAM3: 64KB)
- 内嵌ROM: 30KB (启动代码)

**外设接口**：
- UART1/2/3/4/5 (支持串口下载)
- SPI1/2/3 (可连接外部Flash)
- I2C1/2/3
- USB OTG FS (可选USB升级通道)

**推荐Flash分配**：

```
STM32F407内部Flash分配:
├─ 0x08000000 - 0x08003FFF (16KB) : Bootloader (MCUboot)
├─ 0x08004000 - 0x08023FFF (128KB): Primary Slot (应用1)
├─ 0x08024000 - 0x08043FFF (128KB): Secondary Slot (应用2)
├─ 0x08044000 - 0x0804BFFF (32KB) : Scratch Area
└─ 0x0804C000 - 0x080FFFFF (XXXKB): 保留/存储数据
```

### 3.2 STM32 Flash驱动适配

MCUboot需要以下Flash操作接口：

```c
// flash_map.c 中的关键函数原型 (来自MCUboot)

/**
 * 获取Flash区域信息
 * @param id: Flash区域ID (FLASH_AREA_BOOTLOADER/IMAGE_PRIMARY/IMAGE_SECONDARY等)
 * @param area: 输出的Flash区域信息
 * @return: 0成功, 负数失败
 */
int flash_area_open(uint8_t id, const struct flash_area **area);
void flash_area_close(const struct flash_area *area);

/**
 * 读取Flash
 * @param area: Flash区域
 * @param off: 区域内的偏移
 * @param len: 读取长度
 * @param dst: 目标缓冲区
 * @return: 0成功
 */
int flash_area_read(const struct flash_area *area, uint32_t off, 
                    uint32_t len, void *dst);

/**
 * 写入Flash (无需预先擦除, 仅可写0)
 * @param area: Flash区域
 * @param off: 区域内的偏移
 * @param len: 写入长度
 * @param src: 源缓冲区
 * @return: 0成功
 */
int flash_area_write(const struct flash_area *area, uint32_t off,
                     uint32_t len, const void *src);

/**
 * 擦除Flash扇区
 * @param area: Flash区域
 * @param off: 区域内的偏移 (必须对齐到扇区边界)
 * @param len: 擦除长度 (必须是扇区大小的整数倍)
 * @return: 0成功
 */
int flash_area_erase(const struct flash_area *area, uint32_t off, uint32_t len);

/**
 * 获取Flash的对齐要求 (最小写入单位)
 * @return: 对齐字节数 (通常为1或4)
 */
uint8_t flash_area_align(void);

/**
 * 获取Flash的最大对齐 (用于计算Trailer大小)
 * @return: 字节数
 */
uint16_t flash_area_get_max_align(void);
```

**STM32 Flash驱动实现示例** (使用STM32F4xx HAL库):

```c
// bootutil/src/flash_stm32f4.c (需要创建)

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "bootutil/bootutil_log.h"

// Flash扇区信息表 (STM32F407特有)
static const struct {
    uint32_t start;
    uint32_t size;
} flash_sectors[] = {
    {0x08000000, 16 * 1024},   // Sector 0
    {0x08004000, 16 * 1024},   // Sector 1
    {0x08008000, 16 * 1024},   // Sector 2
    {0x0800C000, 16 * 1024},   // Sector 3
    {0x08010000, 64 * 1024},   // Sector 4
    {0x08020000, 128 * 1024},  // Sector 5-11
    {0x08040000, 128 * 1024},
    {0x08060000, 128 * 1024},
    {0x08080000, 128 * 1024},
    {0x080A0000, 128 * 1024},
    {0x080C0000, 128 * 1024},
    {0x080E0000, 128 * 1024},
};

#define NUM_SECTORS (sizeof(flash_sectors) / sizeof(flash_sectors[0]))

/**
 * 获取Flash区域配置表
 * 由MCUboot内核调用
 */
const struct flash_area flash_areas[] = {
    {
        .fa_id = FLASH_AREA_BOOTLOADER,
        .fa_device_id = 0,
        .fa_off = 0x08000000,
        .fa_size = 16 * 1024,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_PRIMARY,
        .fa_device_id = 0,
        .fa_off = 0x08004000,
        .fa_size = 128 * 1024,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_SECONDARY,
        .fa_device_id = 0,
        .fa_off = 0x08024000,
        .fa_size = 128 * 1024,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_SCRATCH,
        .fa_device_id = 0,
        .fa_off = 0x08044000,
        .fa_size = 32 * 1024,
    },
};

const int flash_area_cnt = sizeof(flash_areas) / sizeof(flash_areas[0]);

/**
 * 读取Flash实现
 */
int flash_area_read(const struct flash_area *area, uint32_t off,
                    uint32_t len, void *dst)
{
    if (off + len > area->fa_size) {
        BOOT_LOG_ERR("Read beyond area bounds");
        return -1;
    }
    
    // 直接从Flash读取 (Flash在内存映射空间)
    uint8_t *src = (uint8_t *)(area->fa_off + off);
    memcpy(dst, src, len);
    return 0;
}

/**
 * 写入Flash实现
 */
int flash_area_write(const struct flash_area *area, uint32_t off,
                     uint32_t len, const void *src)
{
    const uint8_t *src_ptr = (const uint8_t *)src;
    uint32_t write_addr = area->fa_off + off;
    
    // 启用Flash编程
    HAL_FLASH_Unlock();
    
    // 按字(32bit)写入
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t word = *(uint32_t *)(src_ptr + i);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 
                            write_addr + i, word) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }
    
    HAL_FLASH_Lock();
    return 0;
}

/**
 * 擦除Flash实现
 */
int flash_area_erase(const struct flash_area *area, uint32_t off, uint32_t len)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t error_page = 0;
    
    if ((off + len) > area->fa_size) {
        BOOT_LOG_ERR("Erase beyond area bounds");
        return -1;
    }
    
    uint32_t erase_addr = area->fa_off + off;
    
    // 计算要擦除的扇区范围
    uint8_t start_sector = 0xFF, end_sector = 0xFF;
    
    for (int i = 0; i < NUM_SECTORS; i++) {
        if (flash_sectors[i].start == erase_addr) {
            start_sector = i;
        }
        if (flash_sectors[i].start == (erase_addr + len)) {
            end_sector = i;
            break;
        }
    }
    
    if (start_sector == 0xFF || end_sector == 0xFF) {
        BOOT_LOG_ERR("Invalid erase range");
        return -1;
    }
    
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = start_sector;
    erase_init.NbSectors = end_sector - start_sector;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    HAL_FLASH_Unlock();
    
    if (HAL_FLASHEx_Erase(&erase_init, &error_page) != HAL_OK) {
        HAL_FLASH_Lock();
        return -1;
    }
    
    HAL_FLASH_Lock();
    return 0;
}

/**
 * Flash对齐要求
 */
uint8_t flash_area_align(void)
{
    return 4; // 32-bit word对齐
}

uint16_t flash_area_get_max_align(void)
{
    return 8; // MCUboot默认值
}
```

### 3.3 MCUboot配置头文件

创建 `boot/zephyr/include/mcuboot_config/mcuboot_config_stm32f407.h`:

```c
#ifndef MCUBOOT_CONFIG_H
#define MCUBOOT_CONFIG_H

// ============= 基本配置 =============
#define MCUBOOT_LOG_LEVEL MCUBOOT_LOG_LEVEL_INFO

// 签名算法选择 (选一个)
#define MCUBOOT_SIGN_RSA
// #define MCUBOOT_SIGN_EC256
// #define MCUBOOT_SIGN_ED25519

// 镜像加密 (可选)
// #define MCUBOOT_ENC_IMAGES
// #define MCUBOOT_ENCRYPT_RSA
// #define MCUBOOT_ENCRYPT_EC256

// ============= 升级策略 =============
// 交换升级 (推荐)
#define MCUBOOT_SWAP_USING_MOVE
// 或使用: #define MCUBOOT_SWAP_USING_OFFSET

// 覆盖升级 (简单但无回滚)
// #define MCUBOOT_OVERWRITE_ONLY

// 直接XIP (无需交换, 性能最好)
// #define MCUBOOT_DIRECT_XIP

// ============= 验证配置 =============
// 每次启动验证主镜像 (安全性最高, 性能稍差)
#define MCUBOOT_VALIDATE_PRIMARY_SLOT

// 或仅首次验证
// #undef MCUBOOT_VALIDATE_PRIMARY_SLOT

// ============= 串口恢复 =============
#define MCUBOOT_SERIAL
#define BOOT_SERIAL_UART
// #define BOOT_SERIAL_CDC_ACM  // 若使用USB

// ============= 镜像扇区数 =============
#define BOOT_MAX_IMG_SECTORS 128

// ============= FreeRTOS集成 (非Zephyr) =============
// 若使用FreeRTOS而非Zephyr, 定义此值
// #define MCUBOOT_FREERTOS

// ============= 多镜像启动 =============
// 若需要启动多个独立应用
// #define MCUBOOT_IMAGE_NUMBER 2
// #define MCUBOOT_MULTI_IMAGE_SUPPORTED

#endif
```

---

## 第四部分：MCUboot源码架构剖析

### 4.1 核心模块结构

```
mcuboot/
├── boot/                              # 启动应用代码
│   ├── bootutil/                      # 核心库 (与OS无关)
│   │   ├── src/
│   │   │   ├── bootutil.c            # 主逻辑: 镜像选择、验证、交换
│   │   │   ├── image_validate.c      # 镜像验证: 签名检查
│   │   │   ├── image_enc.c           # 镜像解密
│   │   │   ├── swap.c                # 镜像交换实现
│   │   │   ├── loader.c              # 镜像加载
│   │   │   └── keys.c                # 公钥定义
│   │   └── include/
│   │       ├── bootutil/bootutil.h   # 主接口
│   │       ├── bootutil/image.h      # 镜像格式定义
│   │       └── bootutil/flash_map.h  # Flash映射接口
│   │
│   ├── boot_serial/                   # 串口恢复协议
│   │   ├── src/
│   │   │   ├── boot_serial.c         # 串口升级主逻辑
│   │   │   └── mcumgr_transport.c    # MCUmgr协议实现
│   │   └── include/
│   │
│   ├── zephyr/                        # Zephyr OS适配层
│   │   ├── main.c                     # Zephyr bootloader应用入口
│   │   ├── flash_map.c                # Zephyr下的Flash驱动
│   │   ├── prj.conf                   # Zephyr Kconfig配置
│   │   └── CMakeLists.txt
│   │
│   └── mynewt/                        # Apache Mynewt适配层
│
├── scripts/                            # 工具脚本
│   ├── imgtool.py                     # 镜像签名/加密工具
│   ├── keys/                          # 密钥管理
│   │   ├── rsa.py                     # RSA密钥
│   │   ├── ecdsa.py                   # ECDSA密钥
│   │   ├── ed25519.py                 # Ed25519密钥
│   │   └── x25519.py                  # X25519密钥
│   └── requirements.txt               # Python依赖
│
├── ext/                               # 外部依赖
│   ├── mbedtls-3.6.0/                # Mbed TLS密码库
│   ├── tinycbor/                     # CBOR编码库
│   └── ...
│
├── docs/                              # 文档
│   ├── design.md                      # 详细设计文档 (必读!)
│   ├── readme-zephyr.md               # Zephyr集成指南
│   ├── imgtool.md                     # imgtool工具文档
│   ├── encrypted_images.md            # 加密详解
│   └── signed_images.md               # 签名详解
│
└── sim/                               # 模拟器 (测试用)
```

### 4.2 关键启动流程代码分析

**bootutil/src/bootutil.c** - 核心启动逻辑:

```c
/**
 * MCUboot的主入口函数
 * 由各OS的boot应用调用
 * 返回: 应用加载地址
 */
int boot_go(struct boot_rsp *rsp)
{
    // 1. 初始化Flash驱动和加密库
    rc = boot_enc_load_key();
    
    // 2. 对每个镜像执行循环
    for (int i = 0; i < MCUBOOT_IMAGE_NUMBER; i++) {
        // 2.1 检查是否有未完成的交换操作
        rc = boot_swap_check_resume_swap(i);
        if (rc == BOOT_SWAP_TYPE_NONE) {
            // 2.2 检查镜像Trailer, 判断是否需要交换
            rc = boot_swap_status_source(i, &swap_type);
        }
        
        // 2.3 若需要交换, 验证次级镜像
        if (swap_type != BOOT_SWAP_TYPE_NONE) {
            rc = boot_validate_slot(i, BOOT_SLOT_SECONDARY);
            if (rc != 0) {
                // 次级镜像无效, 不执行交换
                swap_type = BOOT_SWAP_TYPE_FAIL;
            }
        }
        
        // 2.4 执行镜像交换
        if (swap_type != BOOT_SWAP_TYPE_NONE) {
            rc = boot_swap_image(i, swap_type);
        }
    }
    
    // 3. 对所有镜像进行依赖检查 (多镜像)
    rc = boot_check_dependencies();
    
    // 4. 验证主镜像
    rc = boot_validate_slot(0, BOOT_SLOT_PRIMARY);
    
    // 5. 加载镜像信息
    rc = boot_load_image(0, &rsp->br);
    
    // rsp->br_image_addr 包含应用加载地址
    return 0;
}
```

**bootutil/src/swap.c** - 镜像交换实现:

```c
/**
 * 执行镜像交换
 * 这是MCUboot最复杂的部分
 */
static int boot_swap_image(int image_index, int swap_type)
{
    // 对于每个镜像扇区:
    // 1. 从Primary读入临时缓冲
    // 2. 从Secondary读入, 写入Primary
    // 3. 从临时缓冲读入, 写入Secondary
    
    // 关键点: 每个步骤后更新Trailer中的交换状态
    //         以便若发生重启, 可恢复交换
    
    for (int i = 0; i < sectors_count; i++) {
        // 保存Primary扇区到Scratch
        flash_area_read(primary, offset, size, buffer);
        flash_area_erase(scratch, 0, size);
        flash_area_write(scratch, 0, size, buffer);
        
        // Secondary → Primary
        flash_area_read(secondary, offset, size, buffer);
        flash_area_erase(primary, offset, size);
        flash_area_write(primary, offset, size, buffer);
        
        // Scratch → Secondary
        flash_area_read(scratch, 0, size, buffer);
        flash_area_erase(secondary, offset, size);
        flash_area_write(secondary, offset, size, buffer);
        
        // 更新交换状态 (存储在Trailer)
        update_swap_status(state_offset, sector_index);
    }
}
```

**bootutil/src/image_validate.c** - 镜像验证:

```c
/**
 * 验证Flash中的镜像
 * 检查完整性和真实性
 */
int boot_validate_slot(int image_index, int slot)
{
    // 1. 读取镜像头部
    flash_area_read(slot, 0, sizeof(image_header), &hdr);
    
    // 2. 检查MAGIC值
    if (hdr.ih_magic != IMAGE_MAGIC) {
        return -1;
    }
    
    // 3. 读取镜像体和TLV区
    image_data = malloc(hdr.ih_img_size);
    tlv_data = malloc(TLV_MAX_SIZE);
    
    // 4. 验证签名 (RSA/ECDSA/ED25519)
    //    使用keys.c中的公钥
    rc = bootutil_verify_sig(image_data, image_size, 
                             tlv_data, &key);
    
    // 5. 验证完整性 (SHA256/384/512)
    rc = bootutil_verify_hash(image_data, hdr.ih_img_size, 
                              tlv_data);
    
    // 6. 检查安全计数器 (防回滚)
    if (hdr.ih_flags & MCUBOOT_SECURITY_COUNTER_ENABLED) {
        if (tlv_security_counter < min_security_counter) {
            return -1; // 镜像版本过老
        }
    }
    
    return 0; // 验证通过
}
```

### 4.3 关键数据结构

**镜像Trailer (存在镜像槽位末尾)**:

```c
// Trailer布局 (从高地址向下):
// ┌─────────────────────────────────────────┐
// │ 交换状态区                               │ (BOOT_MAX_IMG_SECTORS * min-write-size * 3)
// │ = 128 * 1 * 3 = 384 字节                │
// ├─────────────────────────────────────────┤
// │ 加密密钥 0 (16字节)                      │ (可选)
// ├─────────────────────────────────────────┤
// │ 加密密钥 1 (16字节)                      │ (可选)
// ├─────────────────────────────────────────┤
// │ 交换尺寸 (4字节)                         │
// ├─────────────────────────────────────────┤
// │ 未受保护TLV尺寸 Secondary (2字节)       │
// │ 未受保护TLV尺寸 Primary (2字节)         │
// ├─────────────────────────────────────────┤
// │ 交换信息 (1字节)                         │ bit[3:0]=交换类型, bit[7:4]=镜像号
// ├─────────────────────────────────────────┤
// │ COPY-DONE (1字节) 0x01=完成, 0xff=未完成 │
// ├─────────────────────────────────────────┤
// │ IMAGE-OK (1字节) 0x01=确认, 0xff=未确认 │
// ├─────────────────────────────────────────┤
// │ MAGIC (16字节)                           │ 0x77c295f3...或BOOT_MAX_ALIGN编码
// └─────────────────────────────────────────┘
// ↑
// 槽位末尾
```

**镜像头部结构**:

```c
struct image_header {
    uint32_t ih_magic;           // 0x96f3b83d
    uint32_t ih_load_addr;       // 应用加载地址 (已弃用)
    uint16_t ih_hdr_size;        // 头部大小, 通常32字节
    uint16_t ih_protect_tlv_size;// 受保护TLV大小
    uint32_t ih_img_size;        // 镜像大小 (不含头部和TLV)
    uint32_t ih_flags;           // 标志位
    struct image_version ih_ver; // 版本号
    uint32_t _pad1;              // 保留
};

// 镜像标志位定义
#define IMAGE_F_ENCRYPTED_AES128  0x04
#define IMAGE_F_ENCRYPTED_AES256  0x08
#define IMAGE_F_NON_BOOTABLE      0x10  // 分裂镜像
#define IMAGE_F_RAM_LOAD          0x20  // RAM加载
```

---

## 第五部分：Zephyr RTOS集成MCUboot详解

### 5.1 Zephyr项目结构

```
zephyr_project/
├── zephyr/                          # Zephyr内核源码
│   ├── bootloader/
│   │   └── mcuboot/                # 这是MCUboot的副本
│   │       ├── boot/
│   │       ├── scripts/
│   │       └── ...
│   │
│   ├── boards/
│   │   └── stm32/
│   │       └── stm32f407_disco/
│   │           ├── stm32f407_disco.dts    # Device Tree源文件
│   │           ├── stm32f407_disco.yaml   # 板卡配置
│   │           └── CMakeLists.txt
│   │
│   └── samples/
│       └── subsys/mgmt/mcumgr/smp_svr/   # MCUmgr示例
│
├── bootloader/                      # 自定义MCUboot
│   └── mcuboot/
│       ├── boot/
│       │   ├── zephyr/
│       │   ├── bootutil/
│       │   └── ...
│       └── ...
│
└── app/                             # 你的应用
    ├── src/
    │   └── main.c
    ├── prj.conf                    # Kconfig配置
    ├── CMakeLists.txt
    └── boards/
        └── stm32f407_disco.overlay # Device Tree覆盖
```

### 5.2 Device Tree配置 (关键!)

**boards/stm32f407_disco/stm32f407_disco.dts**:

```dts
// STM32F407探索者 - MCUboot Flash分配

/ {
    chosen {
        zephyr,console = &uart1;       // 调试串口
        zephyr,sram = &sram0;
        zephyr,flash = &flash0;
        zephyr,code-partition = &slot0_partition;
    };

    aliases {
        led0 = &user_led;
        sw0 = &user_button;
    };
};

&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        // MCUboot Bootloader
        boot_partition: partition@0 {
            label = "mcuboot";
            reg = <0x00000000 0x4000>;  // 16KB @ 0x08000000
        };

        // 主应用槽位 (Primary)
        slot0_partition: partition@4000 {
            label = "image-0";
            reg = <0x00004000 0x20000>; // 128KB @ 0x08004000
        };

        // 升级槽位 (Secondary)
        slot1_partition: partition@24000 {
            label = "image-1";
            reg = <0x00024000 0x20000>; // 128KB @ 0x08024000
        };

        // 交换临时区
        scratch_partition: partition@44000 {
            label = "image-scratch";
            reg = <0x00044000 0x8000>;  // 32KB @ 0x08044000
        };

        // 可选: 用户数据分区
        storage_partition: partition@4C000 {
            label = "storage";
            reg = <0x0004C000 0x14000>; // 80KB
        };
    };
};

&uart1 {
    status = "okay";
    current-speed = <115200>;
};

// 其他外设配置...
```

**Device Tree说明**:

- `boot_partition`: MCUboot自身代码
- `slot0_partition` (image-0): 主应用程序
- `slot1_partition` (image-1): 升级应用程序
- `scratch_partition`: 交换时临时缓冲
- 所有地址必须按4字节对齐
- 地址从0x00000000开始 (相对于Flash基地址)

### 5.3 Bootloader Kconfig配置

**boot/zephyr/prj.conf** (MCUboot配置):

```kconfig
# ============= 基础配置 =============
CONFIG_BOOTLOADER_MCUBOOT=y

# Cortex-M4处理器
CONFIG_ARM=y
CONFIG_CORTEX_M=y
CONFIG_CORTEX_M4=y

# STM32F407
CONFIG_SOC_STM32F407VG=y
CONFIG_STM32_FLASH_SIZE=1024

# ============= 日志 =============
CONFIG_BOOT_LOG_LEVEL=1  # 0=ERROR, 1=WARNING, 2=INFO, 3=DEBUG

# ============= 签名/加密 =============
# 选择一个签名算法
CONFIG_BOOT_SIGNATURE_TYPE_RSA=y
# CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y
# CONFIG_BOOT_SIGNATURE_TYPE_ED25519=y

# 签名密钥文件 (相对路径)
CONFIG_BOOT_SIGNATURE_KEY_FILE="../../../../my-keys/root-rsa-2048.pem"

# 镜像加密 (可选)
# CONFIG_BOOT_ENCRYPT_AES256=y
# CONFIG_BOOT_ENCRYPTION_KEY_FILE="../../../../my-keys/enc-aes256.pem"

# ============= 升级策略 =============
# 使用交换升级 (推荐)
CONFIG_BOOT_SWAP_USING_MOVE=y

# 或使用覆盖升级
# CONFIG_BOOT_OVERWRITE_ONLY=y

# ============= 验证配置 =============
# 每次启动验证主镜像
CONFIG_BOOT_VALIDATE_SLOT0=y

# ============= 串口恢复 =============
# 启用MCUmgr over UART
CONFIG_MCUBOOT_SERIAL=y
CONFIG_BOOT_SERIAL_UART=y
# CONFIG_BOOT_SERIAL_CDC_ACM=y  # 若使用USB

# 串口恢复等待时间 (毫秒)
# CONFIG_BOOT_SERIAL_WAIT_FOR_DFU=y
# CONFIG_BOOT_SERIAL_WAIT_FOR_DFU_TIMEOUT=5000

# ============= 内存/存储 =============
# 镜像扇区数最大值
CONFIG_BOOT_MAX_IMG_SECTORS=128

# ============= Flash对齐 =============
CONFIG_FLASH_ALIGNMENT=1    # STM32F407支持字节对齐

# ============= 内核配置 =============
CONFIG_KERNEL_INIT_PRIORITY_DEVICE=40

# 减小Bootloader大小
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_LTO=y

# 禁用不需要的功能
CONFIG_ASSERT=n
CONFIG_ASSERT_VERBOSE=n
```

**说明**:

- `CONFIG_BOOT_SIGNATURE_KEY_FILE`: 用来编译Bootloader的密钥
  - 仅提取公钥部分
  - 生产中可使用仅公钥PEM文件
  
- `CONFIG_BOOT_SWAP_USING_MOVE`: 交换升级
  - 需要primary槽位比secondary槽位大一个扇区+交换状态区
  - 或使用 `CONFIG_BOOT_SWAP_USING_OFFSET` (更推荐)
  
- `CONFIG_BOOT_OVERWRITE_ONLY`: 覆盖升级
  - 简单但无法回滚
  - 适合固件大小小的场景

### 5.4 应用Kconfig配置

**app/prj.conf** (应用程序配置):

```kconfig
# ============= MCUboot集成 =============
CONFIG_BOOTLOADER_MCUBOOT=y

# 使用MCUmgr进行OTA更新
CONFIG_MCUMGR=y
CONFIG_MCUMGR_SMP_UART=y          # 串口传输

# 固件管理
CONFIG_MCUMGR_GRP_IMG=y            # 镜像管理命令
CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD=y

# ============= FreeRTOS集成 =============
CONFIG_RTOS_SELECTION_FREERTOS=y
CONFIG_FREERTOS=y

# FreeRTOS堆大小
CONFIG_FREERTOS_HEAP_SIZE_KB=64

# 时钟/定时器
CONFIG_KERNEL_UPTIME_FREQUENCY=1000

# ============= 串口配置 =============
CONFIG_SERIAL=y
CONFIG_UART_CONSOLE=y

# MCUmgr串口 (与console不同的UART)
CONFIG_UART_1=y

# 串口波特率
CONFIG_UART_CONSOLE_BAUD_RATE=115200
CONFIG_UART_1_BAUD_RATE=115200

# ============= 其他 =============
CONFIG_CORTEX_M_SYSTICK=y

# 启用assert
CONFIG_ASSERT=y
```

### 5.5 编译Bootloader

```bash
# 进入Zephyr MCUboot目录
cd zephyr/bootloader/mcuboot/boot/zephyr

# 清理之前的构建
rm -rf build

# 为STM32F407配置并编译
west build -b stm32f407_disco -d build_bootloader

# 或使用完整配置
west build \
    -b stm32f407_disco \
    -d build_bootloader \
    -- \
    -DCONFIG_BOOT_SIGNATURE_TYPE_RSA=y \
    -DCONFIG_BOOT_SIGNATURE_KEY_FILE="../../../my-keys/root-rsa-2048.pem"

# 编译输出
# build_bootloader/zephyr/zephyr.hex  <- 用于烧写
# build_bootloader/zephyr/zephyr.bin  <- 二进制格式
# build_bootloader/zephyr/zephyr.elf  <- 调试信息

# 查看二进制大小
arm-none-eabi-objdump -h build_bootloader/zephyr/zephyr.elf
```

### 5.6 编译应用程序

```bash
# 创建应用项目
mkdir -p ~/mcuboot_demo/app
cd ~/mcuboot_demo/app

# 创建CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20.0)
project(mcuboot_app)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(app PRIVATE src/main.c)
EOF

# 创建prj.conf
# (如上所示)

# 编译应用 (不包含Bootloader)
west build -b stm32f407_disco -d build_app

# 查看应用大小
arm-none-eabi-size build_app/zephyr/zephyr.elf

# 应用二进制
# build_app/zephyr/zephyr.bin
# build_app/zephyr/zephyr.hex
```

---

## 第六部分：FreeRTOS应用开发与MCUboot集成

### 6.1 FreeRTOS基础应用框架

**app/src/main.c** (MCUboot感知的FreeRTOS应用):

```c
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

// 使用Zephyr的GPIO API (工作在FreeRTOS之上)
#include <zephyr/drivers/gpio.h>

#define LED_NODE DT_ALIAS(led0)
#define LED_PIN DT_GPIO_PIN(LED_NODE, gpios)
#define LED_FLAGS DT_GPIO_FLAGS(LED_NODE, gpios)

static const struct device *led_gpio = DEVICE_DT_GET(DT_GPIO_CTLR(LED_NODE, gpios));

// ============= MCUboot集成 =============
#include <mcuboot_status.h>
#include <os/os_malloc.h>

/**
 * 启动完成后, 将应用标记为"已确认"
 * 这样若应用崩溃, MCUboot不会回滚到旧版本
 */
static void boot_app_confirm(void)
{
    int rc = boot_set_confirmed();
    if (rc == 0) {
        LOG_INF("Application image confirmed successfully");
    } else {
        LOG_WRN("Failed to confirm application image (rc=%d)", rc);
    }
}

/**
 * 触发升级: 将Secondary Slot标记为待测试
 * MCUboot下次启动时会交换镜像
 */
static void boot_request_upgrade(void)
{
    int rc = boot_set_pending(false);  // false=一次性测试
    if (rc == 0) {
        LOG_INF("Upgrade marked for next boot");
        // 立即重启以应用新镜像
        sys_reboot(SYS_REBOOT_COLD);
    }
}

/**
 * 获取当前运行的镜像信息
 */
static void print_boot_info(void)
{
    uint32_t image_addr, image_size;
    int rc = boot_app_info(&image_addr, &image_size);
    
    if (rc == 0) {
        LOG_INF("Running image:");
        LOG_INF("  Address: 0x%08x", image_addr);
        LOG_INF("  Size: %u bytes", image_size);
    }
    
    // 获取镜像版本号
    struct image_version ver;
    rc = boot_get_image_info(image_addr, &ver);
    if (rc == 0) {
        LOG_INF("  Version: %u.%u.%u+%u",
                ver.iv_major, ver.iv_minor, 
                ver.iv_revision, ver.iv_build_num);
    }
}

// ============= 应用任务 =============

/**
 * LED闪烁任务 (演示基本FreeRTOS功能)
 */
void led_task(void *arg)
{
    if (!device_is_ready(led_gpio)) {
        LOG_ERR("LED GPIO device not ready!");
        return;
    }
    
    gpio_pin_configure(led_gpio, LED_PIN, GPIO_OUTPUT_ACTIVE);
    
    while (1) {
        gpio_pin_toggle(led_gpio, LED_PIN);
        k_msleep(500);  // 500ms
        
        LOG_DBG("LED toggled");
    }
}

/**
 * 系统监控任务
 */
void monitor_task(void *arg)
{
    uint32_t uptime_s = 0;
    
    while (1) {
        uptime_s = k_uptime_get() / 1000;
        
        LOG_INF("System uptime: %u seconds", uptime_s);
        
        // 运行60秒后自动确认镜像
        if (uptime_s == 60) {
            LOG_INF("Auto-confirming application image...");
            boot_app_confirm();
        }
        
        k_msleep(5000);  // 5秒报告一次
    }
}

/**
 * 串口命令处理任务
 * 支持通过串口升级
 */
void shell_task(void *arg)
{
    const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    
    if (!device_is_ready(uart)) {
        LOG_ERR("UART device not ready!");
        return;
    }
    
    LOG_INF("Shell task started, waiting for commands...");
    LOG_INF("Commands: 'info' - show info, 'upgrade' - trigger upgrade");
    
    // 简单的串口命令处理
    // 生产中应使用更完善的shell框架
}

// ============= 主程序 =============

int main(void)
{
    LOG_INF("MCUboot-aware FreeRTOS Application Starting");
    LOG_INF("===========================================");
    
    // 打印启动信息
    print_boot_info();
    
    // 创建任务
    k_thread_create(&led_thread_data, led_stack, LED_STACK_SIZE,
                   led_task, NULL, NULL, NULL,
                   LED_PRIORITY, 0, K_NO_WAIT);
    
    k_thread_create(&monitor_thread_data, monitor_stack, MONITOR_STACK_SIZE,
                   monitor_task, NULL, NULL, NULL,
                   MONITOR_PRIORITY, 0, K_NO_WAIT);
    
    LOG_INF("All tasks created, system ready");
    
    // Zephyr内核接管调度
    // (不返回)
    
    return 0;
}

// ============= 任务栈和线程对象定义 =============

K_THREAD_STACK_DEFINE(led_stack, 1024);
K_THREAD_STACK_DEFINE(monitor_stack, 1024);

struct k_thread led_thread_data;
struct k_thread monitor_thread_data;

#define LED_PRIORITY 10
#define MONITOR_PRIORITY 9
```

### 6.2 MCUboot集成库 (bootutil_public)

MCUboot提供了 `bootutil_public` 库供应用使用:

```c
// 应用可包含以下头文件:
#include <bootutil/bootutil_public.h>

// 关键API:

/**
 * 获取当前运行镜像的信息
 * @param size: 输出镜像大小
 * @return: 镜像在Flash中的地址, 或-1表示错误
 */
uint32_t boot_get_image_off(uint32_t *size);

/**
 * 标记镜像为"已确认"
 * 防止MCUboot在下次启动时回滚
 * @return: 0成功, 负数失败
 */
int boot_set_confirmed(void);

/**
 * 标记Secondary Slot为待升级
 * MCUboot会在下次启动时交换镜像
 * @param confirm_on_swap: true=永久交换, false=测试性交换
 * @return: 0成功
 */
int boot_set_pending(bool confirm_on_swap);

/**
 * 标记为待测试并重启
 * @return: 不返回 (会重启)
 */
void boot_request_upgrade(bool confirm_on_swap);
```

**应用集成建议**:

1. **启动时**：打印当前运行的镜像版本
2. **初始化后**：若镜像为"测试"状态，进行自检
3. **自检通过**：调用 `boot_set_confirmed()` 确认
4. **收到新镜像**：调用 `boot_set_pending(false)` 并重启
5. **若自检失败**：不调用 `boot_set_confirmed()`，重启时MCUboot会回滚

### 6.3 Serial Recovery / MCUmgr集成

**Zephyr内置MCUmgr支持**:

```kconfig
# app/prj.conf
CONFIG_MCUMGR=y
CONFIG_MCUMGR_SMP_UART=y
CONFIG_MCUMGR_GRP_IMG=y
CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD=y
CONFIG_MCUMGR_GRP_OS=y

# 选择MCUmgr串口 (不同于console)
CONFIG_UART_1=y
CONFIG_MCUMGR_SMP_UART_DEV_NAME="UART_1"
```

**MCUmgr命令行工具** (用于升级):

```bash
# 安装mcumgr工具
pip3 install mcumgr

# 或使用Rust版本
cargo install mcumgr-cli

# ========== 升级流程 ==========

# 1. 生成并签名新固件 (见下章)
./imgtool.py sign -k root-rsa-2048.pem \
  --align 4 --version 1.2.0 \
  --header-size 32 --slot-size 131072 \
  app.bin app_v1.2.0.bin

# 2. 使用mcumgr上传镜像
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image upload app_v1.2.0.bin

# 3. 测试新镜像 (如果支持测试性交换)
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image test <hash>

# 或直接标记为永久
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image confirm <hash>

# 4. 重启应用
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  os reset
```

---

## 第七部分：固件签名与加密详解

### 7.1 生成签名密钥

```bash
# 进入MCUboot脚本目录
cd mcuboot/scripts

# ============= RSA-2048 (推荐) =============
python3 imgtool.py keygen -k root-rsa-2048.pem -t rsa-2048

# 生成密钥对 (~1700字节)
# 包含私钥和公钥

# ============= ECDSA P-256 =============
python3 imgtool.py keygen -k root-ecdsa-p256.pem -t ecdsa-p256

# 生成密钥对 (~600字节)
# 性能比RSA快, 但安全强度相当

# ============= ED25519 =============
python3 imgtool.py keygen -k root-ed25519.pem -t ed25519

# 生成密钥对 (~270字节)
# 最小体积, 但不是所有平台都支持

# ============= 保护私钥 =============
# 可用密码保护私钥
python3 imgtool.py keygen -k root-rsa-2048-pw.pem -t rsa-2048 -p

# 会提示输入密码
# 使用此密钥签名时也需输入密码
```

### 7.2 提取公钥进MCUboot

```bash
# 提取公钥为C代码
python3 imgtool.py getpub -k root-rsa-2048.pem

# 输出:
# const unsigned char root_rsa_2048_pub_key[] = {
#     0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
#     ...
# };
# const unsigned int root_rsa_2048_pub_key_len = 270;

# 将输出复制到 boot/zephyr/keys.c:
# 替换existing的公钥定义

# 或保存为PEM文件 (用于编译MCUboot)
python3 imgtool.py getpub -k root-rsa-2048.pem -e pem > root-rsa-2048-pub.pem

# 检查PEM文件中是否仅包含公钥
python3 imgtool.py keyinfo -k root-rsa-2048-pub.pem
# 应输出: "public"
```

### 7.3 签名应用镜像

**完整签名命令**:

```bash
# 编译得到原始镜像
# zephyr_app.bin (不包含MCUboot头部)

# 签名后输出到 zephyr_app_signed.bin
python3 imgtool.py sign \
  --key root-rsa-2048.pem \              # 私钥文件
  --align 4 \                             # Flash对齐字节数
  --version 1.0.0 \                       # 应用版本
  --header-size 32 \                      # MCUboot头部大小
  --slot-size 131072 \                    # Slot大小 (128KB)
  --pad \                                 # 填充到Slot大小
  zephyr_app.bin \                        # 输入: 原始二进制
  zephyr_app_signed.bin                   # 输出: 签名后的二进制

# 完整参数说明:
# --key: 签名私钥
# --version: 镜像版本号 (major.minor.revision[+build])
# --header-size: 32字节 (固定)
# --slot-size: 最大应用大小 = Primary Slot大小
# --pad: 填充镜像到Slot大小, 添加Trailer
# --align: Flash最小写入单位
#
# 可选参数:
# --test: 标记为测试性镜像 (支持回滚)
# --confirm: 标记为已确认的镜像
# --security-counter N: 防回滚计数器
# --dependencies "(1, 1.0.0)": 声明对其他镜像的依赖
```

**用于升级槽位的签名**:

```bash
# 升级镜像需要额外的 --header-size 偏移

# 方法1: 同样的签名 (推荐)
python3 imgtool.py sign \
  --key root-rsa-2048.pem \
  --align 4 \
  --version 1.1.0 \
  --header-size 32 \
  --slot-size 131072 \
  --pad \
  zephyr_app.bin \
  zephyr_app_v1.1.0_upgrade.bin

# 使用MCUmgr工具上传到Secondary Slot
# (见第6部分)
```

### 7.4 镜像加密

**加密流程**:

1. 生成加密密钥
2. 使用 `imgtool` 加密镜像
3. MCUboot在启动时解密

**生成加密密钥**:

```bash
# 加密使用的私钥 (与签名密钥独立)
python3 imgtool.py keygen -k root-ec-p256.pem -t ecdsa-p256

# 提取公钥供MCUboot编译使用
python3 imgtool.py getpub -k root-ec-p256.pem
```

**加密镜像**:

```bash
# 使用EC256密钥加密镜像 (同时签名)
python3 imgtool.py sign \
  --key root-rsa-2048.pem \              # 签名密钥
  --encrypt root-ec-p256.pem \           # 加密密钥
  --encrypt-keylen 256 \                 # AES-256
  --align 4 \
  --version 1.0.0 \
  --header-size 32 \
  --slot-size 131072 \
  --pad \
  zephyr_app.bin \
  zephyr_app_encrypted.bin

# 生成的镜像将被加密存储在Flash中
# MCUboot会自动检测加密标志并解密
```

**MCUboot加密配置**:

```kconfig
# boot/zephyr/prj.conf
CONFIG_BOOT_ENCRYPT_AES256=y

# 加密密钥文件 (EC256公钥)
CONFIG_BOOT_ENCRYPTION_KEY_FILE="root-ec-p256.pem"
```

### 7.5 防回滚保护

**安全计数器**:

```bash
# 使用安全计数器防止版本回滚
python3 imgtool.py sign \
  --key root-rsa-2048.pem \
  --security-counter auto \              # 从版本自动生成
  --align 4 \
  --version 1.0.0+5 \                    # build=5
  --header-size 32 \
  --slot-size 131072 \
  --pad \
  zephyr_app.bin \
  zephyr_app_v1.0.0_secure.bin

# security-counter会被编码到镜像TLV中
# MCUboot在启动前会验证:
#   新版本的security-counter >= 当前版本
# 否则拒绝升级
```

---

## 第八部分：串口升级完整工作流

### 8.1 硬件连接

```
STM32F407探索者开发板
┌──────────────────────┐
│                      │
│     PA9  (UART1_TX) ──┼──[TX]─→ USB转串口模块
│     PA10 (UART1_RX) ──┼──[RX]←─ USB转串口模块
│                      │
│     GND ─────────────┼──[GND]── USB转串口模块
│                      │
└──────────────────────┘

USB转串口模块 ──USB──→ PC
```

### 8.2 MCUboot序列启动检测

**硬件启动检测** (可选):

某些设计使用GPIO启动引脚进入串口恢复模式:

```c
// boot/zephyr/main.c 中的启动检测代码示例
#include <zephyr/drivers/gpio.h>

#define BOOT_GPIO_NODE DT_ALIAS(sw0)  // User button
#define BOOT_GPIO_PIN DT_GPIO_PIN(BOOT_GPIO_NODE, gpios)

static bool check_serial_recovery_button(void)
{
    const struct device *gpio = DEVICE_DT_GET(
        DT_GPIO_CTLR(BOOT_GPIO_NODE, gpios)
    );
    
    if (!device_is_ready(gpio)) {
        return false;
    }
    
    gpio_pin_configure(gpio, BOOT_GPIO_PIN, GPIO_INPUT);
    
    // 按钮按下 = 进入恢复
    int ret = gpio_pin_get(gpio, BOOT_GPIO_PIN);
    return ret == 0;  // 活跃低电平
}
```

**超时等待启动** (软件方式):

```c
// boot/zephyr/main.c
#include <zephyr/drivers/uart.h>

const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static bool check_dfu_timeout(uint32_t timeout_ms)
{
    uint32_t start = k_uptime_get();
    
    // 等待串口数据
    while (k_uptime_get() - start < timeout_ms) {
        if (uart_irq_update(uart) && uart_irq_is_pending(uart)) {
            // 有数据到达
            return true;
        }
        k_msleep(10);
    }
    
    return false;
}
```

### 8.3 升级完整流程

#### 步骤1: 准备新固件

```bash
# 假设当前版本为 1.0.0, 要升级到 1.1.0

cd mcuboot/scripts

# 编译应用 (得到 app_v1.1.0.bin)
cd ../app
west build -b stm32f407_disco

# 签名镜像
cd ../scripts
python3 imgtool.py sign \
  --key root-rsa-2048.pem \
  --align 4 \
  --version 1.1.0 \
  --header-size 32 \
  --slot-size 131072 \
  --pad \
  ../app/build_app/zephyr/zephyr.bin \
  app_v1.1.0_signed.bin

# 生成升级镜像 (可选, 对于mcumgr)
# 同上即可
```

#### 步骤2: 通过串口上传

**使用MCUmgr工具** (推荐):

```bash
# 列出可用串口
ls /dev/tty* | grep -E "USB|ACM|Serial"
# 或Windows: COM1, COM2, ...

# 上传固件
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image upload app_v1.1.0_signed.bin

# 输出:
# Uploading 131072 bytes
# 100%│████████████████████████████████│ 131072/131072 (0s)
# Image 0 (slot 1) upload complete

# 查看镜像状态
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image list

# 输出示例:
# Image   Slot   Version          Hash
# -----   ----   -------          ----
#   0       0    1.0.0           0x1234...
#   0       1    1.1.0           0x5678...  (Pending)

# 测试新镜像 (进行一次启动测试)
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image test <hash_of_slot1>

# 或直接确认 (永久升级, 无回滚)
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  image confirm <hash_of_slot1>

# 重启以应用升级
mcumgr -c serial,dev=/dev/ttyUSB0,baud=115200 \
  os reset
```

**使用Putty + 手动协议** (低级调试):

```bash
# 若MCUmgr工具不可用, 可使用原始CBOR/SMP协议
# 但建议先使用MCUmgr确认工作流

# 1. 连接Putty到COM端口
#    波特率: 115200
#    数据位: 8
#    停止位: 1
#    奇偶校验: None

# 2. 等待MCUboot启动信息
#    应看到: "[INF] mcuboot: ..."

# 3. MCUmgr will handle the protocol automatically
```

#### 步骤3: 应用确认或回滚

**应用启动后的确认**:

```
Boot Sequence:
1. ┌─ MCUboot启动
   │
   ├─→ 检查Secondary Slot (有新镜像 v1.1.0)
   │
   ├─→ 交换镜像 (Primary ↔ Secondary)
   │   Primary: v1.1.0 (新)
   │   Secondary: v1.0.0 (旧)
   │
   ├─→ 启动Primary中的 v1.1.0
   │
   └─→ 进入应用
   
2. 应用启动
   ├─→ 打印: "Running v1.1.0"
   │
   ├─→ 进行自检 (检查硬件、配置、网络等)
   │
   ├─ 自检通过?
   │  ├─→ YES: 调用 boot_set_confirmed()
   │  │         应用标记为永久
   │  │         下次启动不会回滚
   │  │
   │  └─→ NO: 不调用 boot_set_confirmed()
   │          等待重启...
   
3. 重启发生
   ├─→ MCUboot启动
   │
   ├─ 应用已确认?
   │  ├─→ YES: 正常启动v1.1.0
   │  │
   │  └─→ NO: 进行REVERT操作
   │          交换回v1.0.0
   │          Primary: v1.0.0 (旧)
   │          Secondary: v1.1.0 (失败)
   │          启动v1.0.0
   │
   └─→ Boot complete
```

**应用中的确认代码**:

```c
#include <bootutil/bootutil_public.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_boot, LOG_LEVEL_INF);

/**
 * 获取当前运行的镜像版本号
 */
static int get_current_image_version(
    uint8_t *major, uint8_t *minor, uint16_t *rev)
{
    // MCUboot API: 获取镜像信息
    // (需要bootutil_public库)
    
    uint32_t size;
    uint32_t app_addr = boot_get_image_off(&size);
    
    if (app_addr == 0 || app_addr == (uint32_t)-1) {
        return -1;
    }
    
    // 从Flash读取镜像头部
    // 偏移+8处是镜像版本
    
    return 0;
}

/**
 * 应用自检例程
 */
static int app_self_test(void)
{
    LOG_INF("Performing application self-test...");
    
    // 测试1: 硬件检查
    LOG_INF("  [1/3] Checking hardware...");
    // 检查LED、按钮、外设等
    
    // 测试2: 配置检查
    LOG_INF("  [2/3] Checking configuration...");
    // 验证设置参数的完整性
    
    // 测试3: 网络连接 (如果有)
    LOG_INF("  [3/3] Checking network...");
    // 尝试连接网络、服务器等
    
    LOG_INF("Self-test completed successfully");
    return 0;
}

/**
 * 启动时的boot处理
 */
void on_boot(void)
{
    LOG_INF("Application startup - boot handler");
    
    // 获取当前镜像版本
    uint8_t maj, min;
    uint16_t rev;
    if (get_current_image_version(&maj, &min, &rev) == 0) {
        LOG_INF("Running image version: %u.%u.%u",
                maj, min, rev);
    }
    
    // 执行自检
    if (app_self_test() != 0) {
        LOG_ERR("Self-test failed! Rebooting to previous version...");
        // 不调用boot_set_confirmed(), 让MCUboot回滚
        k_msleep(2000);
        sys_reboot(SYS_REBOOT_COLD);
        return;  // 不会到达这里
    }
    
    // 自检通过, 确认镜像
    LOG_INF("Confirming new image...");
    if (boot_set_confirmed() == 0) {
        LOG_INF("Image confirmed successfully");
    } else {
        LOG_WRN("Failed to confirm image");
    }
}

// 在main()中调用
int main(void)
{
    LOG_INF("Application starting...");
    
    // 启动boot处理
    on_boot();
    
    // 继续正常应用逻辑
    // ...
    
    return 0;
}
```

---

## 第九部分：故障排查与调试

### 9.1 常见问题诊断表

| 现象 | 可能原因 | 排查步骤 |
|------|--------|--------|
| MCUboot无法启动 (卡在启动) | Flash驱动错误 | 检查Flash驱动实现, 验证读写操作 |
| 镜像验证失败 | 签名密钥不匹配 | 确保Bootloader中的公钥与签名使用的私钥配对 |
| 无法进入Serial Recovery | 串口配置错误 | 检查UART管脚配置, 波特率设置 |
| 上传镜像失败 | MCUmgr协议错误 | 运行 `mcumgr -v` 查看详细输出 |
| 升级后应用立即崩溃 | 镜像加载地址错误 | 验证Primary Slot地址与应用链接地址一致 |
| 无法确认镜像 | bootutil_public库缺失 | 添加`CONFIG_BOOTLOADER_MCUBOOT=y`到应用配置 |
| MCUboot循环重启 | Trailer空间不足 | 检查是否有足够的Trailer空间, 调整`BOOT_MAX_IMG_SECTORS` |

### 9.2 调试技巧

**启用详细日志**:

```kconfig
# boot/zephyr/prj.conf
CONFIG_LOG=y
CONFIG_LOG_MODE_IMMEDIATE=y
CONFIG_BOOT_LOG_LEVEL=3          # DEBUG级别

# 可选: 使用RTT输出 (更快)
CONFIG_SEGGER_RTT_CONSOLE=y
```

**JTAG调试**:

```bash
# 连接J-Link到开发板
# 使用Ozone调试器或gdb

# 启动GDB服务器
JLinkGDBServer -device STM32F407VG -endian little -speed 1000

# 另一终端启动gdb
arm-none-eabi-gdb build_bootloader/zephyr/zephyr.elf

(gdb) target remote localhost:2331
(gdb) load
(gdb) break main
(gdb) continue
```

**Flash内容读取**:

```bash
# 使用STMCubeProgrammer读取Flash
STMCubeProgrammer -c port=SWD -r32 0x08000000 128

# 或使用OpenOCD
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

# telnet localhost 4444
> dump_image flash.bin 0x08000000 0x100000
```

**镜像文件分析**:

```bash
# 使用imgtool检查签名镜像
python3 imgtool.py verify -k root-rsa-2048.pem app_signed.bin

# 如果验证通过, 输出镜像信息
# 如果失败, 检查密钥是否正确

# 使用hexdump查看镜像头部
hexdump -C app_signed.bin | head -20

# 应看到MAGIC值: 3d 83 f3 96 (小端)
```

### 9.3 性能优化

**减少Bootloader大小**:

```kconfig
# boot/zephyr/prj.conf
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_LTO=y                    # 链接时优化
CONFIG_LOG_MODE_MINIMAL=y       # 最小日志
CONFIG_BOOT_SERIAL=n            # 禁用串口恢复 (若不需要)
CONFIG_ASSERT=n                 # 禁用断言
```

**加快启动速度**:

```kconfig
# 禁用每次的主镜像验证
# CONFIG_BOOT_VALIDATE_SLOT0=n

# 使用Direct-XIP模式 (无需镜像交换)
# CONFIG_MCUBOOT_DIRECT_XIP=y
```

---

## 第十部分：生产级部署建议

### 10.1 安全最佳实践

**密钥管理**:

```
开发/测试阶段:
├─ 使用MCUboot仓库中的示例密钥 (仅用于开发!)
└─ 存储在代码仓库中

生产前准备:
├─ 生成新的RSA-2048密钥对
│  python3 imgtool.py keygen -k prod-key.pem -t rsa-2048
│
├─ 用密码保护私钥
│  python3 imgtool.py keygen -k prod-key-secure.pem -t rsa-2048 -p
│
└─ 存储规则:
   ├─ 私钥: 仅发布团队可访问 (HSM或USB Key)
   ├─ 公钥: 嵌入到Bootloader (烧写时确定)
   └─ 不要在代码仓库中存储私钥!

工具链集成:
├─ CI/CD系统获取私钥用于签名
├─ 访问控制: 仅授权人员可访问
└─ 审计日志: 记录所有签名操作
```

**启动安全配置**:

```kconfig
# 生产Bootloader配置
CONFIG_BOOT_VALIDATE_SLOT0=y           # 每次验证主镜像
CONFIG_BOOT_SIGNATURE_TYPE_RSA=y       # RSA-2048签名
# CONFIG_MCUBOOT_SERIAL=n              # 禁用串口恢复
# CONFIG_MCUBOOT_SERIAL_DETECTABLE=n   # 禁用可检测的恢复模式

# 应用配置
# 定期调用boot_set_confirmed()
# 实现完整的自检流程
```

### 10.2 版本管理

**版本号编码**:

```
Major.Minor.Patch+BuildNumber

例如: 1.2.3+42

含义:
├─ Major (1): 主要功能变更 (向后不兼容)
├─ Minor (2): 功能增强 (向后兼容)
├─ Patch (3): bug修复
└─ Build (42): 构建编号 (CI/CD自动递增)

MCUboot支持版本比较:
├─ v1.0.0 < v1.0.1 < v1.1.0 < v2.0.0
└─ 防回滚: 新版本必须 >= 旧版本
```

**更新策略**:

```
方案A: 金丝雀更新 (推荐)
├─ 小百分比设备先升级新版本
├─ 如果无故障报告, 扩大范围
└─ 逐步升级到100%

方案B: 分批更新
├─ 按地区/客户批次更新
├─ 每批间隔若干天观察
└─ 有问题快速回滚

方案C: 强制更新
├─ 仅用于安全补丁
├─ 直接确认镜像, 无回滚
└─ 风险高, 需充分测试
```

### 10.3 测试清单

**功能测试**:

- [ ] 正常启动应用
- [ ] 验证镜像签名检查
- [ ] 测试镜像升级
- [ ] 验证镜像回滚 (恢复机制)
- [ ] 测试中断恢复 (升级中断后继续)
- [ ] 验证版本防回滚
- [ ] 串口恢复模式功能

**压力测试**:

- [ ] 重复升级/回滚 (100+次)
- [ ] 模拟升级中的掉电 (多个关键点)
- [ ] Flash磨损测试 (循环擦写)
- [ ] 高温/低温环境测试
- [ ] 应用崩溃自动回滚

**安全测试**:

- [ ] 使用错误密钥签名的镜像被拒绝
- [ ] 篡改镜像数据被检测
- [ ] 旧版本镜像无法降级安装
- [ ] 恢复模式GPIO保护 (防未授权)
- [ ] 防时序攻击 (签名验证)

### 10.4 部署文档模板

创建 `DEPLOYMENT.md`:

```markdown
# MCUboot生产部署指南

## 硬件配置
- 开发板: 正点原子STM32F407探索者
- CPU: ARM Cortex-M4 @ 168MHz
- Flash: 1MB内部Flash
- 分配:
  - Bootloader: 0x08000000-0x08003FFF (16KB)
  - App Primary: 0x08004000-0x08023FFF (128KB)
  - App Secondary: 0x08024000-0x08043FFF (128KB)
  - Scratch: 0x08044000-0x0804BFFF (32KB)

## 初始化烧写 (工厂)

### 1. 烧写Bootloader
```bash
west build -b stm32f407_disco boot/zephyr
st-flash write build/zephyr/zephyr.bin 0x08000000
```

### 2. 烧写初始应用 v1.0.0
```bash
python3 scripts/imgtool.py sign \
  -k prod-key-secure.pem \
  -v 1.0.0 -H 32 -S 131072 --pad \
  app.bin app_v1.0.0.bin

st-flash write app_v1.0.0.bin 0x08004000
```

### 3. 验证
- 观察LED闪烁
- 检查串口输出 "App v1.0.0"
- 应用自检通过

## OTA升级 (野外)

### 升级到v1.1.0
```bash
# 生成升级镜像
python3 scripts/imgtool.py sign \
  -k prod-key-secure.pem \
  -v 1.1.0 -H 32 -S 131072 --pad \
  app_new.bin app_v1.1.0.bin

# 通过MCUmgr上传
mcumgr -c serial,dev=/dev/ttyUSB0 image upload app_v1.1.0.bin

# 测试
mcumgr -c serial,dev=/dev/ttyUSB0 image test <hash>

# 确认
mcumgr -c serial,dev=/dev/ttyUSB0 image confirm <hash>

# 重启
mcumgr -c serial,dev=/dev/ttyUSB0 os reset
```

## 故障回滚

若应用v1.1.0失败 (未调用boot_set_confirmed):
```bash
# MCUboot自动回滚到v1.0.0
# 用户看到: 应用重启 → 回到v1.0.0

# 手动触发回滚 (若需要)
mcumgr -c serial,dev=/dev/ttyUSB0 image erase <slot>
```

## 支持及监控

- 每日上报应用版本及运行时长
- 监控升级成功率
- 异常重启记录
- Flash磨损预警
```

---

## 总结与进阶建议

### 学习路径

**第1阶段：基础理解** ✓
- MCUboot架构设计
- Flash布局和管理
- 镜像格式和验证

**第2阶段：Zephyr集成** (你在这里)
- Device Tree配置
- Bootloader编译
- 应用适配

**第3阶段：FreeRTOS移植**
- MCUboot +FreeRTOS集成
- 应用自检和确认机制
- 串口升级流程

**第4阶段：生产优化**
- 安全加固
- 性能调优
- 故障处理

### 推荐延伸项目

1. **双镜像启动**
   - 同时管理Bootloader和Application两个镜像
   - 实现Bootloader也能升级

2. **外部Flash支持**
   - 将Secondary Slot放在SPI Flash
   - 扩展应用大小空间

3. **网络OTA**
   - 通过WiFi/4G下载镜像
   - 集成HTTP/HTTPS客户端

4. **加密通信**
   - TLS/DTLS加密OTA通道
   - 防止中间人攻击

5. **差分升级** (Delta OTA)
   - 仅发送镜像差异部分
   - 减少传输数据量

### 参考资源

- **MCUboot官方文档**: https://docs.mcuboot.com/
- **Zephyr MCUboot指南**: https://docs.zephyrproject.org/latest/guides/bootloader/

 - **MCUboot源码设计文档**: `docs/design.md`
- **常见问题**: GitHub Issues https://github.com/mcu-tools/mcuboot/issues

---

**这份教程涵盖了MCUboot在STM32上的完整移植过程，从基础概念到生产部署。建议实际动手操作每一个步骤，遇到问题时参考对应的排查章节。成功掌握MCUboot将为你的嵌入式产品开发增添强大的更新能力和鲁棒性。**