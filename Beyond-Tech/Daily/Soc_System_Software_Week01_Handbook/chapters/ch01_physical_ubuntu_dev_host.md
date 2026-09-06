# Chapter 1 - 把物理 Ubuntu mini 主机构造成长期开发 Host

## 1.1 本章只解决一个问题

你的个人笔记本会变化：可能换网络、关机、休眠、带出门。开发环境不能跟着它漂移。

因此本课程把角色固定为：

- **个人笔记本**：编辑器/SSH 客户端/浏览器；
- **Ubuntu mini 主机**：唯一开发 Host；
- **I.MX6ULL MINI / STM32F407**：Target。

本章完成后，你应该可以在笔记本上输入：

```bash
ssh imxdev
```

立即进入 Ubuntu 24.04 Host，并且在 Host 上得到固定的工作目录、工具版本记录、Git 身份、SSH 服务和长期运行终端。

### 本章产物
- `~/work/` 工程目录；
- `~/work/logs/host_baseline_*.txt`；
- 笔记本 `~/.ssh/config` 中的 `Host imxdev`；
- 公钥登录；
- `tmux` 可用于断线不丢编译任务。

---

## 1.2 先形成正确心智模型

### 1.2.1 Host 和 Target 不等于“电脑”和“开发板”

在交叉开发中：

```text
你写代码的机器 = Host
最终运行程序的机器 = Target
```

本课程：

```text
Ubuntu mini PC (x86_64)  --compile--> ARM executable
                                  \
                                   -> i.MX6ULL MINI (ARM Cortex-A7)
```

笔记本只是远程控制 Host，不参与最终编译链。这一点后面分析“为什么我的程序在 Host 能跑、到 Target 不能跑”时非常关键。

### 1.2.2 为什么要把服务放在 Ubuntu Host

后续会一直用：

```text
Git
ARM GCC
Kernel source
U-Boot source
TFTP server
NFS server
Serial terminal
GDB
Zephyr west workspace
Zephyr SDK
```

如果这些组件散落在笔记本和开发机之间，你会不断遇到“文件到底在哪台机器”“谁是 TFTP server”“哪个机器的 IP 是 serverip”等问题。

因此统一规则：

> **除 VS Code 前端/浏览器外，所有编译和 Target 服务均运行在 Ubuntu mini Host。**

---

## 1.3 第一次登录 Host：先识别机器，不要先安装软件

从个人笔记本进入 Ubuntu Host。第一次可以使用真实 IP：

```bash
ssh <ubuntu-user>@<ubuntu-host-ip>
```

登录后立即执行：

```bash
hostnamectl
uname -a
cat /etc/os-release
uname -m
nproc
free -h
lsblk
df -h /
ip -br addr
ip route
```

### 逐条解释

`hostnamectl`
: 查看主机名、系统版本、体系结构。以后日志里只写“Ubuntu”是不够的，要知道是哪台 Host。

`uname -m`
: 应看到类似 `x86_64`。这就是 Host 架构。

`nproc`
: 可用于后面决定 `make -jN` 的并行度。

`free -h`
: 看内存，不要等 Linux Kernel 编译时才发现内存紧张。

`lsblk`
: 确认系统盘和额外数据盘。

`ip -br addr`
: `-br` 是 brief，适合快速看网卡名、UP/DOWN 和 IP。

`ip route`
: 看默认路由。若 Host 自己不能联网，先不要继续装依赖。

### 你要记录什么

建立：

```bash
mkdir -p ~/work/logs
date -Is | tee ~/work/logs/day1_start.txt
hostnamectl | tee -a ~/work/logs/day1_start.txt
uname -a | tee -a ~/work/logs/day1_start.txt
ip -br addr | tee -a ~/work/logs/day1_start.txt
ip route | tee -a ~/work/logs/day1_start.txt
```

---

## 1.4 更新 Ubuntu 24.04，并建立最小工具基线

执行：

```bash
sudo apt update
sudo apt upgrade
```

这里两条命令不是一回事：

- `apt update`：更新“软件包索引”；
- `apt upgrade`：根据新索引升级已安装包。

完成后安装本课程基础工具：

```bash
sudo apt install -y \
    git build-essential make cmake ninja-build \
    gdb gdb-multiarch binutils file \
    strace ltrace tree tmux curl wget \
    openssh-server rsync unzip xz-utils \
    picocom minicom tcpdump ethtool \
    python3 python3-pip python3-venv
```

### 为什么现在就装 `gdb-multiarch`

普通 `gdb` 主要跟 Host 架构配套；后面我们要分析 ARM ELF，所以提前安装 `gdb-multiarch`。Day 2 会验证它。

### 验收

```bash
git --version
gcc --version
cmake --version
python3 --version
gdb-multiarch --version
picocom --help | head
```

只要某个命令提示 `command not found`，就不要把本章标记完成。

---

## 1.5 建立长期目录：以后不要把工程堆在 `$HOME`

创建：

```bash
mkdir -p ~/work/{linux,zephyr,tools,src,logs}
mkdir -p ~/work/linux/{apps,bsp,kernel,uboot,drivers,debug}
mkdir -p ~/work/zephyr/{workspace,apps,boards,logs}
sudo mkdir -p /srv/{tftp,nfs/imx6ull}
```

查看：

```bash
tree -L 3 ~/work
```

建议最终结构：

```text
~/work
├── linux
│   ├── apps
│   ├── bsp
│   ├── debug
│   ├── drivers
│   ├── kernel
│   └── uboot
├── logs
├── src
├── tools
└── zephyr
    ├── apps
    ├── boards
    ├── logs
    └── workspace
```

`/srv/tftp` 与 `/srv/nfs/imx6ull` 属于“Host 对 Target 提供的服务目录”，所以放到 `/srv`，而不是随意散落在工程仓库里。

---

## 1.6 配置 SSH 服务：让笔记本只做前端

### 1.6.1 Host 端确认 sshd

```bash
sudo systemctl enable --now ssh
systemctl status ssh --no-pager
ss -lntp | grep ':22'
```

正常情况下：

- `systemctl` 应显示 `active (running)`；
- `ss` 应看到 `LISTEN ... :22`。

如果 `ssh.service could not be found`：

```bash
sudo apt install openssh-server
sudo systemctl enable --now ssh
```

正点原子旧版《Linux 驱动开发指南》4.2.2 同样要求开启 SSH，只是原文的 Windows->Ubuntu 场景是 SecureCRT。本教程把客户端换成你的个人笔记本，服务端角色没有变化。

### 1.6.2 笔记本生成密钥

在笔记本执行，不是在 Ubuntu Host：

```bash
ssh-keygen -t ed25519 -C "imx6ull-dev"
```

默认保存在：

```text
~/.ssh/id_ed25519
~/.ssh/id_ed25519.pub
```

再执行：

```bash
ssh-copy-id <ubuntu-user>@<ubuntu-host-ip>
```

以后登录不再依赖输入密码。

### 1.6.3 设置 SSH 别名

笔记本编辑：

```text
~/.ssh/config
```

加入：

```sshconfig
Host imxdev
    HostName 192.168.10.10
    User YOUR_USER
    ServerAliveInterval 30
    ServerAliveCountMax 3
```

`192.168.10.10` 只是课程示例。必须换成 Day 1 实际记录的 Ubuntu Host 地址。

验证：

```bash
ssh imxdev
```

### 1.6.4 VS Code Remote SSH

笔记本安装 VS Code 的 `Remote - SSH` 扩展后，连接 `imxdev`。

判断是否真的在远程开发，不要看窗口颜色，要打开 VS Code 终端执行：

```bash
hostname
pwd
uname -m
```

它们必须显示 Ubuntu Host 的信息。

---

## 1.7 为什么还要 `tmux`

SSH 断开并不应该终止一次 30 分钟内核编译。

Host：

```bash
tmux new -s dev
```

在 tmux 中运行：

```bash
watch -n 1 date
```

按：

```text
Ctrl+b
然后按 d
```

detach。

断开 SSH，再重新：

```bash
ssh imxdev
tmux attach -t dev
```

如果 `watch` 还在运行，你就理解了“SSH session”和“Host 上的进程”不是一回事。

后续内核、Yocto、Zephyr 下载都建议在 tmux 中运行。

---

## 1.8 为开发 Host 固定网络身份

最省维护成本的方案是：

> **在路由器/DHCP Server 上给 Ubuntu Host 的 MAC 地址做 DHCP Reservation。**

这样 Host 仍通过 DHCP 获取配置，但地址长期固定。例如：

```text
Ubuntu Host : 192.168.10.10
Laptop      : DHCP，例如 192.168.10.30
I.MX6ULL    : 192.168.10.20（开发静态地址）
Gateway     : 192.168.10.1
Mask        : 255.255.255.0
```

先找 Host 有线网卡 MAC：

```bash
ip -br link
```

再看：

```bash
ip link show <your-ethernet-iface>
```

不要在没有确认接口名之前复制 `eth0`、`ens33`、`enp2s0` 这类名字。

---

## 1.9 故障实验：主动把 SSH 问题分层

不要真正停掉远程 Host 的 sshd，除非你有本地键盘屏幕。做无破坏实验：

### 实验 A：错误 HostName

临时执行：

```bash
ssh YOUR_USER@192.168.10.250
```

观察：

```text
Connection timed out
```

这表示 TCP 连接都没建立，和 SSH key 无关。

### 实验 B：正确 IP、错误用户名

```bash
ssh definitely_wrong_user@<ubuntu-host-ip>
```

这时网络/TCP 已经成立，问题进入认证层。

建立下面的排障顺序：

```text
1. ping Host IP
2. nc -vz HostIP 22
3. ssh -vvv user@HostIP
4. Host: systemctl status ssh
5. Host: journalctl -u ssh
```

---

## 1.10 本章验收

在不看教程的情况下完成：

```text
Laptop
  -> ssh imxdev
  -> Ubuntu Host
  -> tmux new/attach
  -> ~/work
```

然后执行：

```bash
bash ~/work/tools/capture_host_baseline.sh
```

如果你按章节顺序学习，可把本包 `tools/` 下的脚本复制到 `~/work/tools/`。

### 口述检查
1. 为什么笔记本不是本课程 Host？
2. 为什么 TFTP/NFS 应该跑在 Ubuntu mini 主机？
3. SSH 断线为什么不应导致内核编译退出？
4. `ip addr` 和 `ip route` 分别回答什么问题？

答不上来，不进入 Day 2。

---

## 1.11 原始资料

- `ALI-DRV-1.5.2`：《I.MX6U 嵌入式 Linux 驱动开发指南 V1.5.2》**第4章，4.2.2 SSH 服务开启**：[PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- Ubuntu OpenSSH Server：[官方文档](https://ubuntu.com/server/docs/openssh-server)
- 正点原子文档总仓库：[GitHub Archive](https://github.com/alientek-openedv/imx6ull-document)

> 正点原子旧教程使用 Windows + VMware；本章只继承其“Linux Host 需要 SSH/NFS/工具链”的工程角色，不复制虚拟机方案。你的 Host 是物理 Ubuntu 24.04 mini 主机。
