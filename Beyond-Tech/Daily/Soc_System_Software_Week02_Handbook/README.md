# Week 2 - BSP Build Loop and First Zephyr Board Port

> 这周按“做出来”组织，不按理论章节组织。
>
> 目标：
> 1. 自己编出正点原子 I.MX6ULL MINI 的 U-Boot / Kernel / DTB / modules；
> 2. 用 TFTP 启动刚编译的 Kernel + DTB，不烧写；
> 3. 完成一次 ARM 用户态远程 GDB；
> 4. 创建 Explorer STM32F407 out-of-tree Zephyr board；
> 5. 把 clock / USART1 console / LED 跑起来。

## 章节

| Day | Chapter | 任务 | 文档 |
|---|---|---|---|
| 1 | Ch.8 | 整理 BSP 源码、版本、工具链和产物路径 | [Open](chapters/ch08_imx6ull_bsp_artifact_map.md) |
| 2 | Ch.9 | 编译 U-Boot，定位 `u-boot.imx` | [Open](chapters/ch09_build_vendor_uboot.md) |
| 3 | Ch.10 | 编译 Kernel / DTB / modules | [Open](chapters/ch10_build_kernel_dtb_modules.md) |
| 4 | Ch.11 | 用 TFTP 启动新 Kernel + DTB，不烧写 | [Open](chapters/ch11_tftp_boot_kernel_dtb.md) |
| 5 | Ch.12 | `gdbserver + gdb-multiarch` 远程调试 | [Open](chapters/ch12_remote_gdb_userspace.md) |
| 6 | Ch.13 | 创建 Explorer F407 out-of-tree Zephyr board | [Open](chapters/ch13_create_zephyr_board.md) |
| 7 | Ch.14 | Clock + USART1 console + LED bring-up；Week 2 gate | [Open](chapters/ch14_f407_bringup_and_week2_gate.md) |

## 可直接复制的实验文件

### Linux
- [capture_bsp_baseline.sh](tools/capture_bsp_baseline.sh)
- [stage_kernel_to_tftp.sh](tools/stage_kernel_to_tftp.sh)
- [gdb demo](labs/gdb_demo/README.md)

### Zephyr
- [Board starter](zephyr_app/boards/others/alientek_f407_explorer/)
- [Smoke application](zephyr_app/)
- [Explorer schematic](references/Explorer_STM32F4_V2.2_SCH.pdf)
- [Manual index](references/README.md)

## Week 2 完成标志

```text
[ ] U-Boot source/version/defconfig/output 路径明确
[ ] Kernel source/version/defconfig/zImage/DTB/modules 路径明确
[ ] TFTP boot 新 Kernel + DTB 成功
[ ] boot log 能证明运行的是本周构建产物
[ ] 至少完成一次 gdbserver remote breakpoint，或明确记录 target 无 gdbserver
[ ] Zephyr 能发现 alientek_f407_explorer
[ ] 自定义 board clean build 成功
[ ] USART1/CH340 出现 Zephyr console
[ ] LED0/LED1 smoke test 成功
```

## 主要资料

- [正点原子公开文档仓库](https://github.com/alientek-openedv/imx6ull-document)
- [4.3 编译出厂 U-Boot](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/u-boot/)
- [4.4 编译出厂 Kernel/DTB/modules](https://wiki.alientek.com/docs/Boards/Linux/IMX6U/I.MX6U%20%E5%BF%AB%E9%80%9F%E4%BD%93%E9%AA%8C%E6%89%8B%E5%86%8C/cross%20compiling/comple_core/)
- [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- [Zephyr Custom Board](https://docs.zephyrproject.org/latest/develop/application/index.html#custom-board-devicetree-and-soc-definitions)
