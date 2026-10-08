# Q:
1. 基于mcuboot的app，是不是只能采用代码在flash执行这种方式？
2. 同时，bootloader占据的ram是否就不能被app使用了？

# A:
两个问题的直接回答是：

1. **绝对不是！** MCUboot 完全支持 **RAM 加载执行（Ramload）** 模式，并不强制要求 APP 必须在 Flash 原位执行（XIP, Execute In Place）。
2. **完全可以被 APP 使用！** Bootloader 跳转后生命周期即告结束，它占用的 RAM **100% 可以被 APP 完全回收并覆盖使用**。

下面为你展开深度解析这两个问题的底层架构原理与实现细节：

---

### 问题一：基于 MCUboot 的 APP，只能在 Flash 执行（XIP）吗？

#### 1. 两种运行模式对比

* **模式 A：Flash 原位执行（XIP, Execute-In-Place）—— 默认最常用**
  * **机制**：APP 代码保存在 Flash 的 Primary Slot（主槽），CPU 的指令总线直接从 Flash 读取指令并执行。
  * **优点**：节省 RAM 空间；系统启动速度快（不需要耗时往 RAM 搬运代码）。
  * **缺点**：Flash 读取速度受到 CPU 主频限制（STM32F4 需要配置 Flash Wait States / 插入等待周期），且如果 APP 在运行期间需要擦写片上 Flash（如擦写相邻 Sector 存参数），可能会导致 CPU 指线总线 Stall（阻塞）。

* **模式 B：RAM 加载执行模式（Ramload / RAM Execution）—— 完全支持**
  * **机制**：MCUboot 官方原生支持配置宏 `MCUBOOT_RAM_LOADING`。MCUboot 在完成签名校验后，会将 Primary Slot 中的代码完整的**搬运（Copy）到指定的 SRAM/SDRAM 地址**，然后跳转到 RAM 中去执行 APP。
  * **应用场景**：
    1. **性能极致要求**：代码在 RAM 中运行无等待周期，主频性能发挥到极限（特别适合 DSP 计算、AI 边缘推理、电机高频控制）。
    2. **QSPI / External Flash 升级**：固件保存在外挂的 QSPI Flash / NOR Flash 中，启动时搬到片内高带宽 SRAM 或外挂 PSRAM 运行。
    3. **运行时 Flash 擦写**：APP 运行期间需要频繁对片上 Flash 进行大块擦写，代码在 RAM 中运行可避免 Flash 访问冲突。

#### 2. 如何启用 MCUboot 的 RAM 运行模式？

在 `mcuboot_config.h` 中，MCUboot 提供了专门的宏定义：

```c
/* 开启 RAM 加载模式 */
#define MCUBOOT_RAM_LOADING 1

/* 定义 APP 在 RAM 中运行的起始基地址及最大允许尺寸 */
#define MCUBOOT_IMAGE_RAM_SCHEMA 1
```

**编译时的配合改动**：
* **APP 的链接脚本（`.ld`）**：APP 的 `.text`、`.rodata` 段的 **VMA（虚拟内存地址/运行地址）** 必须写成 RAM 地址（例如 STM32F4 的 CCM RAM `0x10000000` 或 SRAM `0x20000000`），而 **LMA（加载地址）** 依旧是 Flash 地址。
* **imgtool 打包**：使用 `imgtool` 签名时，传入 `--load-addr` 参数，指定该镜像需要被搬运的目标 RAM 地址：
  ```bash
  imgtool sign \
      --key root-ec256.pem \
      --header-size 0x200 \
      --load-addr 0x20000000 \ # 明确告诉 MCUboot：验签通过后，把镜像搬到这个 RAM 地址
      --slot-size 0x60000 \
      app_raw.bin app_signed.bin
  ```

---

### 问题二：Bootloader 占据的 RAM，APP 是否还能使用？

#### 1. 核心结论：100% 可以全额回收利用！

MCUboot 并不是操作系统，它与 APP 之间**不是**“父进程与子进程”的关系，而是**“前任与后任”的单向替代关系**。

当 Bootloader 执行完验签、Swap、准备跳转的最后一步时，它的历史使命就彻底完成了。它**不会**在后台常驻内存（除非你主动设计了常驻内存的 Boot-Service）。

#### 2. 底层回收机制（ARM Cortex-M 启动流程解析）

为什么 APP 能够毫无顾忌地覆盖 Bootloader 留在 RAM 里的数据？

1. **堆栈指针（MSP）重置**：
   在跳转前，Bootloader 会提取 APP 向量表的第一项（即 APP 的初始 MSP 地址），并执行：
   ```c
   __set_MSP(*(__IO uint32_t *)app_vector_addr);
   ```
   这会将主堆栈指针直接强制指回 APP 自己划定的栈顶（例如 `0x20020000`）。Bootloader 之前压栈的所有局部变量、函数调用栈帧全部被丢弃。

2. **APP C 运行时环境（Startup）重新初始化**：
   当 CPU 跳转到 APP 的 `Reset_Handler` 后，APP 自己的启动汇编代码（`startup_stm32f407xx.s`）会干两件事：
   * 将 Flash 中的 `.data` 段复制到 RAM。
   * **将 RAM 中的 `.bss` 段（所有全局未初始化变量、静态变量）全部清零（Zero-Init）**。

   这个过程会**全面物理覆盖** Bootloader 在 RAM 里留下的所有残余数据（变量、哈希缓存、TinyCrypt 上下文等）。

#### 3. 大厂工程实践中的“特例”：跨 Bootloader-APP 共享 RAM 区域

在某些生产级复杂业务中，工程师**主动要求** Bootloader 和 APP 共享一小块 RAM（通常几十到几百字节），用于传递关键信息。

* **典型应用场景**：
  1. **传参/复位原因**：Bootloader 告诉 APP：“这次启动是因为刚做完 OTA Swap 升级，请执行初始化校验”。
  2. **快速热启动/安全密钥共享**：Bootloader 校验通过后，将解密后的敏感 Key 保存在 RAM 某区域，传给 APP 使用，避免 APP 再次解密。
  3. **Bootloader 进刷机模式标志**：APP 发生异常挂看门狗前，在 RAM 写入 `0xDEADBEEF`，重启后 Bootloader 读取该标志并留存在 Boot 状态不跳 APP。

* **如何划定共享 RAM 防止被 APP 的 `.bss` 清零清除？**
  需要在链接脚本（`.ld`）中单独划出一个专门的 Section（如 `.noinit`），**禁止 APP 启动代码对其清零**：

  ```ld
  MEMORY
  {
    /* 划出 64 字节专门用于 Boot 与 APP 共享传参 */
    SHARED_RAM (rw) : ORIGIN = 0x20000000, LENGTH = 64
    /* APP 实际可用的普通 RAM 从 0x20000040 开始 */
    RAM        (xrw) : ORIGIN = 0x20000040, LENGTH = 128K - 64
  }

  SECTIONS
  {
    .shared_data (NOLOAD) :
    {
      *(.shared_data)
    } > SHARED_RAM
  }
  ```

---

### 总结对照表

| 维度                    | Flash 原位执行 (XIP)                             | RAM 加载执行 (Ramload)                               |
| :-------------------- | :------------------------------------------- | :----------------------------------------------- |
| **MCUboot 机制**        | 校验后直接跳转到 Primary Slot 的 Flash 地址             | 校验后将镜像由 Flash 拷贝至 RAM 再跳转                        |
| **执行速度**              | 受 Flash 访问等待周期（Wait State）限制                 | **零等待周期**，达 CPU 最高理论性能                           |
| **RAM 占用**            | 仅占用 APP 的 `.data` 和 `.bss` 及 Stack/Heap      | 需额外占用能**完整容纳整个 `.text` 代码段**的 RAM                |
| **Bootloader RAM 回收** | **100% 回收**（跳转后 Bootloader RAM 空间全部被 APP 覆盖） | **100% 回收**（Bootloader 原 RAM 空间被 APP 拿去当代码段或数据段） |
