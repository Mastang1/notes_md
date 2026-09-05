# Environment Baseline

## 1. 主环境

```text
Windows 10/11 Host
└─ VMware Workstation
   └─ Ubuntu 24.04 LTS x86_64
      ├─ Linux/BSP toolchain
      ├─ TFTP/NFS/SSH/GDB
      └─ Zephyr 4.4.1 workspace + Zephyr SDK
```

选择 Ubuntu 24.04 的原因：Zephyr 当前官方 Getting Started 覆盖 Ubuntu 24.04 LTS 及以上；Linux BSP 的旧依赖如果不兼容，放入容器/次级 VM，不反过来把主学习环境锁死在旧发行版。

Zephyr 官方：
https://docs.zephyrproject.org/latest/develop/getting_started/

## 2. VM 建议

- CPU：4 vCPU 起，主机资源充足可 8 vCPU；
- RAM：8 GB 起，后续 Yocto 建议 16 GB+；
- Disk：100 GB 起，后续 Yocto/Kernel/Zephyr 并存建议 200 GB；
- Network：桥接作为开发板互联默认模式；
- USB：允许串口/JTAG 按需直通；
- Snapshot：完成 Day 1 后做 `baseline-clean` 快照。

## 3. Workspace

```text
~/work/
├── src/          # source trees
├── toolchains/   # standalone cross compilers
├── nfs/          # target shared files/rootfs
├── tftp/         # U-Boot downloads
├── logs/         # raw logs
└── course/       # your own exercises/README
```

Zephyr 官方 workspace 保持其推荐结构：

```text
~/zephyrproject/
├── .venv/
├── .west/
├── zephyr/
└── modules/ ...
```

## 4. 版本基线文件

每周运行：

```bash
{
  date -Is
  uname -a
  lsb_release -a
  gcc --version | head -1
  cmake --version | head -1
  ninja --version
  python3 --version
  git --version
  ip -br addr
  ip route
} | tee ~/work/logs/host-baseline.txt
```

交叉编译器、Kernel、U-Boot、Zephyr 的 commit/tag 也必须进入各自 README。

## 5. 旧 BSP 依赖原则

如果正点原子旧 Kernel/U-Boot 在 Ubuntu 24.04 因 Python2、旧 host GCC、库版本失败：

1. 先记录**具体报错**；
2. 判断是 BSP 源码问题还是 Host compatibility；
3. 使用 Docker/Ubuntu 20.04 次级环境隔离；
4. 主环境继续保持 24.04。

不要为了一个旧脚本把全局环境改成不可维护状态。
