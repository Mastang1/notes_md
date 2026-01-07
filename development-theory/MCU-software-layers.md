# 1. 嵌入式软件的基本分层

| **步骤**           | **所在的层**                                          | **角色**   | **具体的动作与“内心独白”**                                                                                                               | **数据形态**                                          |
| ---------------- | ------------------------------------------------- | -------- | ------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------- |
| **1**            | **APP 层**                                         | **指挥官**  | "我现在需要当前的室温，单位要摄氏度。"<br><br>  <br><br>`float temp = BSP_Get_Temperature();`                                                    | **业务数据**<br><br>  <br><br>(25.5°C)                |
| **$\downarrow$** |                                                   |          |                                                                                                                                |                                                   |
| **2**            | **Board Driver 层**<br><br>  <br><br>(BSP/ECU Abs) | **翻译官**  | "老板要温度。我知道板子上挂的是 **LM75** 传感器，地址是 **0x90**，接在 **I2C1** 上。我要发读取命令。"<br><br>  <br><br>`HAL_I2C_Read(I2C1, 0x90, &raw_data, ...)` | **器件指令**<br><br>  <br><br>(Addr: 0x90, Cmd: Read) |
| **$\downarrow$** |                                                   |          |                                                                                                                                |                                                   |
| **3**            | **Chip Driver 层**<br><br>  <br><br>(MCAL/HAL)     | **发报员**  | "收到发送任务。我不管你是发给温度计还是存储器。我只负责翻转 **PB6/PB7** 引脚电平，产生符合 I2C 协议的时序。"                                                               | **协议波形**<br><br>  <br><br>(Start, Ack, Stop Bits) |
| **$\downarrow$** |                                                   |          |                                                                                                                                |                                                   |
| **4**            | **Hardware 层**                                    | **物理线路** | (电压在 PCB 铜线上高低变化)                                                                                                              | **电信号**<br><br>  <br><br>(3.3V / 0V)              |
# 2. BSP vs Driver
---

# 🏛️ 嵌入式架构师笔记：BSP vs Driver

> **核心定义：**
> 
> - **Driver (驱动层/HAL):** 解决 **"怎么说话"** 的问题（如何操作芯片内部的控制器）。
>     
> - **BSP (板级支持包):** 解决 **"跟谁说话"** 的问题（板子上接了什么设备，怎么管理它们）。
>     

---

## 1. 核心差异对比表 (The Contrast)

|**维度**|**Driver (驱动 / HAL / LL)**|**BSP (板级支持包)**|
|---|---|---|
|**物理对象**|**MCU 芯片** (Silicon)|**PCB 电路板** (Board)|
|**关注范围**|芯片内部外设 (GPIO, I2C, SPI, DMA)|板载外部设备 (LED, Sensor, LCD, Flash)|
|**通用性**|**高** (同系列芯片通用，如 STM32F4xx)|**极低** (仅限特定 PCB 版本)|
|**依赖文件**|芯片参考手册 (Reference Manual)|电路原理图 (Schematic), BOM 表|
|**API 风格**|**机制型** (Mechanism)<br><br>  <br><br>`I2C_Write(Addr, Data)`|**策略/业务型** (Policy)<br><br>  <br><br>`EEPROM_SaveConfig(Config)`|
|**变更频率**|极低 (除非换芯片厂家)|高 (改版、换料、引脚调整)|
|**典型文件名**|`stm32f4xx_hal_spi.c`|`bsp_spi_flash.c`|

---

## 2. 架构分层视图

作为架构师，在画图时应严格区分这两层，严禁“跨层调用”（App层直接跳过 BSP 操作 Driver）。

代码段

```
graph TD
    App[APP 应用层] --> |调用业务接口| BSP
    
    subgraph "板级相关 (Board Specific)"
    BSP[BSP 层]
    BSP_H[bsp.h / bsp_led.c / bsp_key.c]
    end
    
    BSP --> |调用传输接口| Driver
    
    subgraph "芯片相关 (Chip Specific)"
    Driver[Driver 层 / HAL]
    HAL[stm32_hal_gpio.c / stm32_hal_uart.c]
    CMSIS[寄存器定义 / CMSIS]
    end
    
    Driver --> |读写寄存器| HW_MCU
    BSP -.-> |物理电气连接| HW_Board
    
    HW_MCU[MCU 硬件 (Silicon)]
    HW_Board[PCB 外设 (LED, Sensor...)]
```

---

## 3. 详细设计指南

### 🔧 A. Driver 层设计 (The "Mechanism")

**宗旨：** 只提供单纯的操作手段，**绝对不包含**任何板级具体的配置参数（如：不要在 Driver 里写死 "PA5 引脚"）。

- **功能职责：**
    
    1. **寄存器抽象：** 将 `0x40021018` 这种地址映射为 `GPIO->BSRR`。
        
    2. **原子操作：** 保证设置寄存器位时的原子性。
        
    3. **时钟/中断管理：** 提供开启外设时钟、清除中断标志位的标准接口。
        
    4. **传输协议实现：** 实现 I2C 时序、SPI 读写、UART 波特率计算。
        
- **架构师的 Check List：**
    
    - [ ] Driver 代码里是否出现了具体的引脚号（如 `Pin_5`）？如果有，**删掉**，那是 BSP 该干的事。
        
    - [ ] Driver 代码是否依赖了特定的传感器型号？如果有，**解耦**。
        

### 🛠️ B. BSP 层设计 (The "Policy")

**宗旨：** 屏蔽电路板细节。即使电路板重新 Layout，只要功能没变，BSP 对上提供的接口就不应该变。

- **功能职责：**
    
    1. **引脚映射 (Pin Muxing):** 决定 I2C1 使用 PB6/PB7 还是 PB8/PB9。
        
    2. **外设初始化链:** 按顺序初始化时钟、GPIO、中断优先级、DMA。
        
    3. **外部器件驱动:** 包含 LM75、OLED、EEPROM 等具体元器件的驱动代码。
        
    4. **功能语义化:** 将 `SetPin(Low)` 封装为 `Relay_On()`。
        
- **架构师的 Check List：**
    
    - [ ] `main.c` 里面是否出现了 `HAL_GPIO_Write`？如果有，**重构**，封装进 BSP。
        
    - [ ] 换了不同型号的温度传感器，应用层代码需要修改吗？如果需要，说明 **BSP 封装失败**。
        

---

## 4. 实战代码对比： "抽象的艺术"

### 场景：读取一个气压传感器的数据

#### ❌ 糟糕的设计 (无 BSP 或 BSP 职责不清)

应用层代码直接与硬件驱动纠缠，换个板子这行代码就废了。

C

```
// main.c (应用层)
void main() {
    // 😱 灾难：APP 开发者需要知道传感器挂在 I2C2 上，地址是 0xEE
    uint8_t cmd = 0xF1;
    HAL_I2C_Master_Transmit(&hi2c2, 0xEE, &cmd, 1, 100); 
    
    uint8_t data[2];
    HAL_I2C_Master_Receive(&hi2c2, 0xEE, data, 2, 100);
    
    // 😱 灾难：APP 开发者还需要知道具体的计算公式
    float pressure = (data[0] << 8 | data[1]) / 4096.0; 
}
```

#### ✅ 优秀的设计 (BSP 完美分层)

应用层只关注业务（气压），不关注硬件（I2C, 0xEE, 4096）。

C

```
// 1. Driver 层 (STM32 HAL库) - 只有 ST 官方提供
// void HAL_I2C_Master_Transmit(...); 

// 2. BSP 层 (bsp_pressure.c) - 架构师定义的接口
// 包含：具体的 I2C 句柄、传感器地址宏、数据转换公式
float BSP_Pressure_Get_kPa(void) {
    // 内部处理具体的 I2C2, 0xEE, 原始数据转换
    // 如果以后换成 SPI 接口的传感器，只有这里需要改
    return sensor_value; 
}

// 3. App 层 (main.c) - 干净、清爽
void main() {
    float current_pressure = BSP_Pressure_Get_kPa();
    if (current_pressure > 101.3) {
        // 业务逻辑...
    }
}
```

---

## 5. 架构师总结 (The Takeaway)

- **Driver 是手脚**：强壮、标准、通用。
    
- **BSP 是神经末梢**：适配特定的环境（电路板），感知具体的冷暖。
    
- **App 是大脑**：只思考逻辑，不操心具体的肌肉如何收缩。
    

**设计口诀：**

> 变硬件不动 App，动 App 不看硬件。
> 
> 中间这道墙，就是 BSP。