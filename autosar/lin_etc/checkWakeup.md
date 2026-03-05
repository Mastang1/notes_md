
## 1. AUTOSAR中，checkWakeup的调用流程

### AUTOSAR LIN MCAL `CheckWakeup` 调用流程笔记

---

## 一、问题背景

在 AUTOSAR 架构中，LIN 总线的唤醒检测涉及电源管理与通信栈的协同。  
`Lin_CheckWakeup()` 属于 MCAL 层接口，其调用必须符合 AUTOSAR 分层规范。

关键原则：

- 唤醒事件属于 ECU 电源管理域
    
- MCAL 仅负责硬件检测
    
- 唤醒状态机由 EcuM 管理
    
- 应用层不得直接调用 MCAL
    

---

## 二、规范调用链（符合 AUTOSAR SWS）

标准调用路径：

```
MCU Wakeup ISR
    ↓
EcuM_CheckWakeup()
    ↓
LinIf_CheckWakeup()
    ↓
Lin_CheckWakeup()
    ↓
LinIf_WakeupConfirmation()
    ↓
EcuM_SetWakeupEvent()
```

模块职责划分：

|模块|职责|
|---|---|
|EcuM|唤醒源管理、状态机控制|
|LinIf|通信接口适配|
|Lin Driver (MCAL)|硬件寄存器检测|
|MCU|硬件唤醒中断|

---

## 三、典型场景说明

场景：ECU 处于 Sleep → LIN 总线发送 Wakeup Pulse → ECU 被唤醒

触发链条：

1. LIN 收发器产生 WK 信号
    
2. MCU 产生唤醒中断
    
3. ISR 调用 `EcuM_CheckWakeup(ECUM_WKSOURCE_LIN)`
    
4. 逐层传递至 MCAL 检测寄存器
    
5. 检测成功后通知 EcuM
    
6. EcuM 状态机切换至 RUN
    

---

## 四、关键伪代码示例（符合 AUTOSAR 分层）

### 1）MCU 唤醒中断

```c
ISR(MCU_Wakeup_ISR)
{
    EcuM_CheckWakeup(ECUM_WKSOURCE_LIN);
}
```

---

### 2）EcuM 层

```c
void EcuM_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
{
    if (WakeupSource & ECUM_WKSOURCE_LIN)
    {
        LinIf_CheckWakeup(WakeupSource);
    }
}
```

---

### 3）LinIf 层

```c
void LinIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
{
    uint8 ControllerId;

    for (ControllerId = 0; ControllerId < LINIF_MAX_CONTROLLERS; ControllerId++)
    {
        if (LinIf_Config[ControllerId].WakeupSource == WakeupSource)
        {
            if (Lin_CheckWakeup(ControllerId) == E_OK)
            {
                EcuM_SetWakeupEvent(WakeupSource);
            }
        }
    }
}
```

---

### 4）MCAL 层（Lin Driver）

```c
Std_ReturnType Lin_CheckWakeup(uint8 Channel)
{
    Std_ReturnType ret = E_NOT_OK;

    if (LIN_HW[Channel].STATUS & LIN_WAKEUP_FLAG)
    {
        LIN_HW[Channel].STATUS = LIN_WAKEUP_FLAG;   // 清除硬件标志
        LinIf_WakeupConfirmation(Channel);
        ret = E_OK;
    }

    return ret;
}
```

---

### 5）回调确认

```c
void LinIf_WakeupConfirmation(uint8 Channel)
{
    EcuM_WakeupSourceType source;
    source = LinIf_Config[Channel].WakeupSource;

    EcuM_SetWakeupEvent(source);
}
```

---

## 五、状态机转换（EcuM）

```
SLEEP
   ↓
WAKEUP VALIDATION
   ↓
RUN
```

EcuM 负责验证唤醒源合法性，然后驱动 ECU 进入正常运行状态。

---

## 六、工程实现注意点

### 1）禁止应用层直接调用 MCAL

错误示例：

```c
Lin_CheckWakeup(0);   // 不符合 AUTOSAR 分层
```

原因：

- 绕过电源管理状态机
    
- 破坏分层架构
    
- 影响 ASIL 设计一致性
    

---

### 2）硬件实现差异

在部分 MCU（如：

- NXP S32K 系列
    
- Infineon TC3xx 系列
    

）中：

`Lin_CheckWakeup()` 实际检测来源可能包括：

- LIN 控制器状态寄存器
    
- LIN 收发器 WK 引脚（通过 Port 或 ICU 读取）
    

即唤醒源 = 控制器标志 + 收发器硬件信号

这也是为什么 AUTOSAR 将 Wakeup 管理放在 EcuM，而非通信模块内部。

---

## 七、设计思想总结

1. 唤醒是电源管理事件，不是通信事件
    
2. MCAL 只负责“检测”
    
3. 状态机与决策权属于 EcuM
    
4. LinIf 仅做适配与转发
    
5. 整个链条必须可追溯、可验证
    

---

## 八、完整时序图（简化）

```
LIN Bus Wakeup
        ↓
Transceiver WK
        ↓
MCU ISR
        ↓
EcuM_CheckWakeup()
        ↓
LinIf_CheckWakeup()
        ↓
Lin_CheckWakeup()
        ↓
EcuM_SetWakeupEvent()
        ↓
ECU RUN
```

---

这份笔记可以直接用于：

- BSW 设计说明文档
    
- MCAL 集成说明
    
- 唤醒路径 Code Review
    
- AUTOSAR 面试准备
    

如果需要，我可以再补充一版“带 SchM 关键区保护 + DET/DEM 错误上报”的量产级实现笔记版本。

## 2. 大致总结

wakeup flow中：
checkWakeup > confirm, 校验，然后ECU发送事件，不在这里执行具体的处理流。