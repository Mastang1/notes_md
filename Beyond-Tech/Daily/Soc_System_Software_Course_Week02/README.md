# Week 02 Tutorial Package

主题：**i.MX6ULL Build Loop + ALIENTEK STM32F407 Zephyr Board Port**。

本包按 20 周重构计划 Week 2 编写，文件名全部使用 ASCII/English；正文保持中文教学。

## Recommended order

1. `02_linux/week02/W02D01_iMX6ULL_BSP_Artifact_Map.md`
2. `02_linux/week02/W02D02_iMX6ULL_Full_Build_UBoot_Kernel_DTB.md`
3. `02_linux/week02/W02D03_TFTP_Boot_Kernel_and_DTB.md`
4. `02_linux/week02/W02D04_Remote_GDB_User_Space_Debugging.md`
5. `03_zephyr/week02/W02D05_Zephyr_Out_of_Tree_Board_Creation.md`
6. `03_zephyr/week02/W02D06_STM32F407_Clock_Console_and_Flashing.md`
7. `02_linux/week02/W02D07_Week2_Clean_Build_and_DT_Comparison.md`

## Week Gate

通过标准：

- i.MX6ULL 能独立构建 U-Boot、Kernel、DTB；
- 可通过 TFTP 从 RAM 启动 Kernel/DTB，不修改持久存储；
- 用户态 remote GDB 闭环完成；
- Zephyr out-of-tree `f407_explorer/stm32f407xx` board 能被发现、编译；
- Explorer F407 能通过正确 clock/UART/pinctrl 输出 console；
- 能准确解释 Linux 与 Zephyr Devicetree 的消费流程差异。

## Dependencies

本 Week 默认你已经完成 Week 1，并使用课程根目录中的：

- `00_course_guide/source_index.md`
- `00_course_guide/hardware_inventory.md`
- `04_deep_dive/A01_DeviceTree_From_DTS_to_Linux_Device.md`
- `references/Explorer STM32F4_V2.2_SCH.pdf`
