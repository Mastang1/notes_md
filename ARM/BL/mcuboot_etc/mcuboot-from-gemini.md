# 🚀 基于 STM32F407 的 MCUboot 深度移植与 OTA 开发指南

## 第一阶段：架构设计与 Flash 分区映射（核心基础）

MCUboot 并非传统原厂提供的简单 Bootloader，它是一个功能强大的安全引导程序，支持防掉电刷写（Swap/Scratch机制）、降级保护、多镜像管理以及非对称加密签名（RSA/ECDSA）。

  

STM32F407ZGT6 拥有 1MB Flash。与一般的 MCU 不同，**F4 系列的 Flash 扇区大小是不均匀的**（前 4 个 16KB，1 个 64KB，后续均为 128KB）。这在移植 MCUboot 时是一个巨大的坑，因为 MCUboot 默认偏好均匀扇区来进行 Swap。

  

### 1.1 Flash 物理分区表设计

为了兼容 MCUboot 的 Swap 机制，主槽（Primary Slot）和备用槽（Secondary Slot）的大小必须绝对一致。Scratch（交换区）的大小必须至少等于最大的物理扇区（128KB）。

  

|**分区名称 (MCUboot 术语)**|**物理扇区范围**|**偏移地址 (起始)**|**大小**|**功能描述**|
|---|---|---|---|---|
|**Bootloader**|Sector 0 - 3|`0x08000000`|64 KB|存放 MCUboot 核心代码及加密公钥|
|**Primary Slot** (Slot 0)|Sector 4 - 6|`0x08010000`|320 KB|运行的主应用程序 (FreeRTOS 固件)|
|**Secondary Slot** (Slot 1)|Sector 7 - 9|`0x08060000`|320 KB|下载的新固件 (待升级槽位)|
|**Scratch Area**|Sector 10|`0x080B0000`|128 KB|Swap 交换区，用于断电恢复|
|**User Data**|Sector 11|`0x080D0000`|128 KB|留给用户存配置或日志（本案例不用）|

## 第二阶段：MCUboot Bootloader 核心移植 (裸机工程)

不要试图在 FreeRTOS 里跑 Bootloader。Bootloader 工程应该是基于 HAL 库的裸机工程。

  

### 2.1 提取 MCUboot 核心代码

从 Git 仓库拉取代码后，你只需要保留以下目录到你的 STM32 CubeIDE 或 Keil 工程中：

  

- `boot/bootutil/src/`：MCUboot 核心逻辑（`bootutil_misc.c`, `image_validate.c`, `loader.c` 等）。
    
      
    
- `boot/bootutil/include/`：头文件。
    
      
    
- `ext/tinycrypt/`：极力推荐使用 tinycrypt 替代 mbedtls 进行签名校验（ECDSA-secp256r1），代码体积极小。
    
      
    

### 2.2 实现 Flash Map 抽象层 (Flash HAL)

MCUboot 依赖外界提供 Flash 的读、写、擦除接口。你需要创建 `flash_map_backend.c` 并实现 `boot/bootutil/include/flash_map_backend/flash_map_backend.h` 中的接口。

  

**核心接口实现范例：**

  

C

```c
#include "flash_map_backend/flash_map_backend.h"
#include "stm32f4xx_hal.h"

// 将逻辑区域转换为物理地址
static uint32_t get_flash_address(const struct flash_area *area, uint32_t off) {
    return area->fa_off + off;
}

// 擦除 Flash (必须按照 STM32F4 的扇区规则)
int flash_area_erase(const struct flash_area *area, uint32_t off, uint32_t len) {
    uint32_t start_addr = get_flash_address(area, off);
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    
    HAL_FLASH_Unlock();
    // 【勘误点】：此处必须编写一个函数根据 start_addr 算出所在的 STM32 物理扇区号 (FLASH_SECTOR_4 等)
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = GetSectorNumber(start_addr); 
    EraseInitStruct.NbSectors = GetNumberOfSectors(start_addr, len); 
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        return -1;
    }
    HAL_FLASH_Lock();
    return 0;
}

// 写入 Flash
int flash_area_write(const struct flash_area *area, uint32_t off, const void *src, uint32_t len) {
    uint32_t dest_addr = get_flash_address(area, off);
    const uint8_t *data = (const uint8_t *)src;
    
    HAL_FLASH_Unlock();
    for (uint32_t i = 0; i < len; i += 1) {
        // STM32F4 推荐按 Byte/Half-word/Word 写入，此处按 Byte 演示，实际工程建议按 Word 提速
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, dest_addr + i, data[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }
    HAL_FLASH_Lock();
    return 0;
}

// 读取 Flash (STM32 可直接通过指针读取内存映射区)
int flash_area_read(const struct flash_area *area, uint32_t off, void *dst, uint32_t len) {
    uint32_t src_addr = get_flash_address(area, off);
    memcpy(dst, (const void *)src_addr, len);
    return 0;
}
```

### 2.3 配置 mcuboot_config.h

在该头文件中定义 MCUboot 的行为模式：

  

C

```
#define MCUBOOT_VALIDATE_PRIMARY_SLOT  // 每次启动都校验主槽签名
#define MCUBOOT_USE_TINYCRYPT          // 使用 TinyCrypt 库
#define MCUBOOT_SIGN_EC256             // 使用 ECDSA P-256 签名算法
#define MCUBOOT_SWAP_USING_SCRATCH     // 使用 Scratch 区域进行安全的 Swap 升级
#define MAX_FLASH_ALIGN 1              // STM32F4 支持按字节写入，此处设为 1
```

### 2.4 主程序与跳转逻辑 (Bootloader `main.c`)

Bootloader 的核心流：初始化硬件 -> 调用 `boot_go` 获取主槽应用信息 -> 清理外设环境 -> 修改 PC 与 SP 跳转。

  

C

```c
#include "bootutil/bootutil.h"
#include "bootutil/image.h"

static void jump_to_application(uint32_t image_address) {
    typedef void (*pFunction)(void);
    pFunction JumpToApplication;
    uint32_t JumpAddress;

    // 1. 禁用所有中断
    __disable_irq();

    // 2. 恢复外设时钟和寄存器到默认状态 (极其重要！)
    HAL_RCC_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    
    // 【勘误点】清理所有悬挂中断，否则 FreeRTOS 启动时会莫名其妙 HardFault
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 3. 获取 App 的栈顶地址并设置
    // 注意：image_address 是 Slot 0 的起始地址，但带有 Header，真正的代码从 Header 之后开始
    // 我们假设 Header size 是 0x200 (512字节)
    uint32_t vector_table_addr = image_address + 0x200; 
    
    JumpAddress = *(__IO uint32_t*) (vector_table_addr + 4);
    JumpToApplication = (pFunction) JumpAddress;
    __set_MSP(*(__IO uint32_t*) vector_table_addr);

    // 4. 起飞！
    JumpToApplication();
}

int main(void) {
    HAL_Init();
    SystemClock_Config(); // 配置时钟
    
    struct boot_rsp rsp;
    // boot_go() 是 MCUboot 的核心状态机。它会检查签名、执行拷贝/Swap
    if (boot_go(&rsp) == 0) {
        // 返回成功，rsp.br_image_off 就是应当启动的镜像槽位偏移（通常是 Primary Slot 偏移）
        // STM32F4 Flash 基地址是 0x08000000
        jump_to_application(0x08000000 + rsp.br_image_off);
    }
    
    // 如果 boot_go 失败，说明没有合法的应用程序，进入死循环等待 JTAG 救援
    while (1) {}
}
```

## 第三阶段：FreeRTOS 应用开发与 OTA 适配

应用的开发是一个标准的带 FreeRTOS 的 STM32 工程（可以通过 STM32CubeMX 直接生成，加入串口收发逻辑）。

  

### 3.1 修改链接脚本 (`.ld` 文件 或 Keil 的 Scatter File)

应用不再从 `0x08000000` 启动，而是从 Primary Slot 开始，**并且必须预留 MCUboot Image Header 的空间**。

  

我们在架构中规划：Primary Slot 地址是 `0x08010000`，大小 320KB (`0x50000`)。

规定 MCUboot Header 大小为 `0x200`（512 Bytes）。

  

**GCC 链接脚本 (`STM32F407ZGTx_FLASH.ld`) 修改：**

  

代码段

```c
/* 原始的 FLASH 定义 */
/* FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K */

/* MCUboot 改造后的 FLASH 定义 */
/* 应用程序真实起始地址 = Slot 0 地址 + Header 大小 = 0x08010200 */
/* 应用程序可用长度 = Slot 0 总长度 - Header 大小 = 320K - 0x200 = 327168 Bytes */
FLASH (rx) : ORIGIN = 0x08010200, LENGTH = 327168
```

### 3.2 向量表重定向 (VTOR)

在 `main()` 函数的最开头（或者 `SystemInit()` 中），必须将 VTOR 寄存器指向新的向量表起始地址：

  

C

```c
int main(void) {
    // 0x08010000 (Slot0起始) + 0x200 (Header偏移)
    SCB->VTOR = 0x08010200; 
    
    HAL_Init();
    SystemClock_Config();
    // 初始化 FreeRTOS 并启动调度器...
}
```

_【勘误点】：VTOR 寄存器赋值必须是对齐的（通常至少是 512 字节对齐）。`0x08010200` 是 512 对齐的，完全符合 ARM Cortex-M4 要求。_

  

### 3.3 FreeRTOS 下的串口 OTA 任务逻辑

在探索者开发板上，建立一个 FreeRTOS 任务，通过 USART1 接收固件。你可以使用 YMODEM 协议或者自定义简单的分包协议。

  

**应用层 OTA 核心逻辑：**

  

1. 建立 TCP/UDP 或串口连接，准备接收 `.bin` 固件。
    
      
    
2. 将接收到的固件切片，**写入到 Secondary Slot (`0x08060000`) 中**。
    
      
    
3. 固件接收并写入完成后，**关键操作：向 Secondary Slot 的尾部（Trailer）写入 Magic Word 和标志位**。
    
      
    
4. 调用 `NVIC_SystemReset()` 重启 MCU。
    
      
    
5. Bootloader 启动，检测到 Trailer 中的 Magic Word，验证新固件签名，执行 Swap 覆盖旧固件，启动新固件。
    
      
    

**写入 MCUboot Flag 的核心代码：**

MCUboot 判断是否需要升级，是通过检查 Slot 的最后几个字节（Trailer）。在应用层接收完固件后，必须执行以下 C 代码：

  

C

```c
#include "stm32f4xx_hal.h"

#define SECONDARY_SLOT_ADDR   0x08060000
#define SLOT_SIZE             (320 * 1024) // 320KB
#define TRAILER_MAGIC_OFFSET  (SLOT_SIZE - 16)
#define IMAGE_OK_OFFSET       (SLOT_SIZE - 24)

// 触发下一次重启时 MCUboot 执行升级
void trigger_mcuboot_upgrade(void) {
    // MCUboot 要求的 16 字节 Magic Word (不可更改)
    const uint32_t mcuboot_magic[4] = {
        0xf395c277,
        0x7fefd260,
        0x0f505235,
        0x8079b62c,
    };
    
    HAL_FLASH_Unlock();
    // 写入 Pending 标志位 (让 Bootloader 知道这里有一个待升级固件)
    // 根据 MCUboot 协议，这里通常写 Image Ok = 0x01 或者直接写入 Magic
    for(int i=0; i<4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 
                          SECONDARY_SLOT_ADDR + TRAILER_MAGIC_OFFSET + i*4, 
                          mcuboot_magic[i]);
    }
    HAL_FLASH_Lock();
    
    // 软件重启
    NVIC_SystemReset();
}
```

## 第四阶段：使用 imgtool 工具链打包与签名

FreeRTOS 应用编译出来的原始二进制文件（`app.bin`）是不能直接跑的，必须使用 MCUboot 官方的 Python 工具 `imgtool` 增加 Header，并使用私钥签名。

  

### 4.1 安装工具

Bash

```
pip install imgtool
```

### 4.2 生成 ECDSA 私钥

Bash

```
imgtool keygen -k ecdsa-p256.pem -t ecdsa-p256
```

_(同时，你需要把这个公钥导出为 C 数组，放在 Bootloader 代码中用于验签)_：

  

Bash

```c
imgtool getpub -k ecdsa-p256.pem -l c
```

### 4.3 给 FreeRTOS App 签名打包

编译出 `app.bin` 后，执行以下命令：

  

Bash

```c
imgtool sign \
    --key ecdsa-p256.pem \
    --align 1 \
    --version 1.0.0 \
    --header-size 0x200 \
    --pad-header \
    --slot-size 0x50000 \
    app.bin app_signed.bin
```

**参数深度解析（面试必考）：**

  

- `--align 1`：与 Bootloader 里的 `MAX_FLASH_ALIGN` 匹配（STM32可以按字节写入，设为 1）。
    
      
    
- `--header-size 0x200`：强行把 App 实体向后推 512 字节，留给 MCUboot 填入元数据和签名哈希。
    
      
    
- `--pad-header`：在文件开头补 0 撑满这 512 字节（极其重要，否则你的中断向量表就错位了）。
    
      
    
- `--slot-size 0x50000`：320KB，告诉工具计算 Trailer 的偏移量。
    
      
    

得到的 `app_signed.bin`，就是最终要通过串口发给板子的文件，也可以在工厂模式下直接烧录到 `0x08010000`。

  

## 第五阶段：完整业务流验证与测试步骤

1. **出厂烧录**：
    
      
    - 将 Bootloader 烧录到 `0x08000000`。
        
          
        
    - 将编译好并被 `imgtool` 签名打包的 `app_signed.bin` 烧录到 `0x08010000`。
        
          
        
2. **正常启动**：
    
      
    - 开发板上电。
        
          
        
    - Bootloader 验证 Primary Slot 的签名（成功）。
        
          
        
    - Bootloader 跳转到 `0x08010200`。
        
          
        
    - FreeRTOS 系统启动，串口打印 "App V1.0.0 Running"。
        
          
        
3. **串口 OTA 升级**：
    
      
    - 修改代码，变更为 V1.1.0，重新编译并用 `imgtool` 生成 `app_v1_1_signed.bin`（version改1.1.0）。
        
          
        
    - 通过串口调试助手或 Python 脚本，将该文件发送给开发板。
        
          
        
    - FreeRTOS 内的串口任务接收文件，写入 Secondary Slot (`0x08060000`)。
        
          
        
    - 写入 Trailer Magic Word，触发重启。
        
          
        
4. **Bootloader Swap**：
    
      
    - Bootloader 启动，发现 Secondary Slot 尾部有 Magic。
        
          
        
    - Bootloader 对比主槽和备用槽，验证新固件签名。
        
          
        
    - **Bootloader 开始将 Primary 和 Secondary 通过 Scratch (Sector 10) 区域分块交换（Swap）**。
        
          
        
    - 交换完成，跳转回 `0x08010200`。
        
          
        
    - FreeRTOS 新版本启动，打印 "App V1.1.0 Running"。
        
          
        

## 第六阶段：深度勘误与面试加分项（💡 求职必杀技）

在面试场上，你可以主动抛出你在做这个项目时解决的几个“底层痛点”，这些能让你瞬间拉开与普通应用工程师的差距：

  

### 1. F4 的“巨大”扇区造成的性能痛点（Flash Wear & Erase Time）

**问题**：F407 的 5~11 扇区每个高达 128KB。在 MCUboot Swap 期间，如果中途断电，MCUboot 会通过 Scratch 恢复。由于扇区大，擦除一个 128KB 扇区需要 1-2 秒，期间不能响应任何中断。

**你的方案（亮点）**：提出现有设计的局限性。如果是量产产品，面对这样的大扇区，我会建议将 MCUboot 降级为 `Overwrite Only`（仅覆盖）模式而不是 Swap 模式，牺牲回滚功能，来换取极高的升级稳定性和速度。MCUboot 原生支持开启 `MCUBOOT_OVERWRITE_ONLY` 宏即可。

  

### 2. FreeRTOS 系统奔溃（外设未反初始化）

**问题**：很多新手在写 Bootloader 调 `JumpToApplication` 时，直接改 PC 指针，导致 App 中的 FreeRTOS 创建任务后开启 SysTick 瞬间死机。

**你的方案（亮点）**：深刻理解 ARM 中断架构，在跳转前不仅关闭全局中断 `__disable_irq()`，更清空了 NVIC 的所有挂起寄存器 `ICPR` 和清除所有外设时钟 `HAL_RCC_DeInit()`，确保 App 能够像硬件冷启动一样拿到一个干净的 CPU 环境。

  

### 3. Flash 双Bank（Dual-Bank）架构探讨

**面试官若追问：如果 MCU 换成 STM32G4 或是 F427 这种支持 Dual-Bank 的怎么做 OTA？**

**你的回答**：MCUboot 是软件级 Swap。对于 Dual-Bank MCU，我会放弃 MCUboot 的软件复制机制，转而利用硬件的 Boot 别名映射。App 直接接收固件放在 Bank2，然后修改硬件 Option Bytes 切换 Boot Bank 重启，这被称为 A/B 升级，速度是 MCUboot 的数倍。这展现了你不仅懂轮子，更懂底层硬件与轮子的边界。

  

**总结**：

完成这个项目后，你的简历上可以大胆写上：**“主导并完成基于 MCUboot 的 STM32F4 安全引导层开发。实现非对称加密签名校验、Flash 非均等扇区适配、防掉电 Swap 回滚机制，并在 FreeRTOS 层面实现了 YMODEM 串口热更新通道的打通，熟悉 ARM Cortex-M 异常处理、VTOR重定向及内存映射机制。”**

  

这套技术栈，直接对标当前各大厂 IoT 设备（如智能家居、新能源BMS、车联网网关）真实量产级的 OTA 方案，祝你求职顺利！