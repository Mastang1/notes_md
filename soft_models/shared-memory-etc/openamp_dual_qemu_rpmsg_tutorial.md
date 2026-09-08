# OpenAMP 实战教程：Dual-QEMU + IVSHMEM RPMsg Demo 与源码快速分析

> 目标读者：熟悉 ARM、RTOS 内核、共享内存 IPC，尤其熟悉 NXP `ipc-shm` 的嵌入式开发工程师。  
> 教程目标：先按 OpenAMP 官方仓库把一个**可实际运行的 RPMsg 数据面 Demo**搭起来，再沿着一次 `rpmsg_send()` 把 RPMsg / VirtIO / VirtQueue 的核心实现追通。  
> 本文不从 remoteproc 开始，也不先读 libmetal 全部源码。

---

## 0. 版本基线与重要说明

本文以 OpenAMP 官方 `openamp-system-reference` 为 Demo 来源：

- 仓库：<https://github.com/OpenAMP/openamp-system-reference>
- 本文固定参考 release：`v2026.04.0`
- Demo：`examples/zephyr/dual_qemu_ivshmem/`
- release manifest：`west.yml` 中 Zephyr 固定为 `v4.4.0`
- Zephyr SDK：建议让 `west sdk install` 安装当前与 Zephyr 4.4 匹配的 SDK；OpenAMP 在升级 Zephyr 4.4 的 PR 中实际构建日志使用过 SDK `1.0.1`

对应源码：

- `west.yml`：<https://github.com/OpenAMP/openamp-system-reference/blob/v2026.04.0/west.yml>
- Demo README：<https://github.com/OpenAMP/openamp-system-reference/blob/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/README.rst>
- Host：<https://github.com/OpenAMP/openamp-system-reference/tree/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/host>
- Remote：<https://github.com/OpenAMP/openamp-system-reference/tree/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/remote>
- IVSHMEM backend：<https://github.com/OpenAMP/openamp-system-reference/tree/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/rpmsg_ivshmem_backend>

### 0.1 官方文档存在版本漂移

`v2026.04.0` 的 release 已经把 `west.yml` 更新到：

```yaml
- name: zephyr
  revision: v4.4.0
```

但同一个 release 中 `dual_qemu_ivshmem/README.rst` 的 prerequisites 仍写：

```text
Tested with Zephyr version 3.4.0
Tested with Zephyr SDK 0.16.1
```

因此本文采用以下原则：

1. **源码与 manifest 优先于旧 README 的版本文字。**
2. Demo 的启动顺序、`ivshmem-server` 参数、构建命令和 shell 测试命令仍严格依据该 README。
3. `west.yml` 中 `open-amp`、`libmetal`、`openamp-zephyr-modules` 使用的是 `main`，不是 SHA，所以第一次成功 `west update` 后必须冻结 manifest，避免以后更新到不同版本。

冻结当前 workspace：

```bash
cd ~/work/openamp_ws
west manifest --freeze -o west-frozen.yml
```

以后做源码分析、项目复现时，把 `west-frozen.yml` 与实验记录一起保存。

---

# Part I：把官方 Dual-QEMU RPMsg Demo 跑起来

## 1. 这个 Demo 实际模拟了什么

官方 Demo 结构如下：

```text
Ubuntu PC

┌──────────────────────────────┐
│ QEMU #1: Cortex-A53          │
│ Zephyr                       │
│ OpenAMP RPMsg HOST           │
└──────────────┬───────────────┘
               │
               │ VirtIO / VirtQueue
               │
        ┌──────▼──────┐
        │  IVSHMEM    │
        │ shared mem  │
        │ + doorbell  │
        └──────▲──────┘
               │
               │ VirtIO / VirtQueue
               │
┌──────────────┴───────────────┐
│ QEMU #2: Cortex-A53          │
│ Zephyr                       │
│ OpenAMP RPMsg REMOTE         │
└──────────────────────────────┘
```

它可以真实验证：

```text
rpmsg_create_ept()
        ↓
rpmsg_send()
        ↓
RPMsg header / buffer
        ↓
VirtIO
        ↓
VirtQueue / vring
        ↓
IVSHMEM shared memory
        ↓
IVSHMEM doorbell
        ↓
peer virtqueue_notification()
        ↓
RPMsg endpoint callback
```

它**不验证** Linux `remoteproc` 加载 R5/M7 固件，也不等价于真实 SoC 的 cache / Mailbox / IPI 行为。它的价值是把 OpenAMP 数据面先独立跑通。

对比你熟悉的 NXP `ipc-shm`：

```text
OpenAMP Demo                       NXP ipc-shm
---------------------------------------------------------------
RPMsg endpoint                 ≈   channel
RPMsg buffer                   ≈   IPC buffer
VirtIO descriptor              ≈   ipc_shm_bd
VirtQueue/vring                ≈   BD/ring queue
ivshmem shared memory          ≈   reserved shared memory
ivshmem_int_peer()             ≈   ipc_hw_irq_notify()
virtqueue_notification()       ≈   RX IRQ 后的 ipc_shm_rx()
endpoint_cb()                  ≈   channel RX callback
```

---

## 2. 推荐主机环境

为了减少 Zephyr 版本问题，建议直接使用：

```text
Ubuntu 24.04 LTS x86_64
Python >= 3.12
CMake  >= 3.28
Git
Ninja
```

Zephyr 当前 Getting Started 对 Ubuntu 24.04+ 的依赖安装命令如下。

### 2.1 安装主机依赖

```bash
sudo apt update
sudo apt install --no-install-recommends \
    git cmake ninja-build gperf ccache \
    dfu-util device-tree-compiler wget \
    python3-dev python3-venv python3-tk \
    xz-utils file make gcc gcc-multilib \
    g++-multilib libsdl2-dev libmagic1
```

检查：

```bash
python3 --version
cmake --version
git --version
ninja --version
```

验收：

```text
Python >= 3.12
CMake  >= 3.28
```

如果 Ubuntu 版本较老，最省时间的方案不是手工修几十个 Python/CMake 依赖，而是直接使用 Ubuntu 24.04 的 VM/WSL/迷你 PC 环境。

---

## 3. 创建 OpenAMP workspace

统一使用以下目录：

```text
~/work/openamp_ws
```

执行：

```bash
mkdir -p ~/work/openamp_ws
cd ~/work/openamp_ws

python3 -m venv .venv
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install west
```

检查：

```bash
which python
which west
west --version
```

正常情况下应该类似：

```text
/home/<user>/work/openamp_ws/.venv/bin/python
/home/<user>/work/openamp_ws/.venv/bin/west
```

**以后每次重新打开终端，都先执行：**

```bash
cd ~/work/openamp_ws
source .venv/bin/activate
```

---

## 4. 获取官方 `openamp-system-reference`

执行：

```bash
cd ~/work/openamp_ws

git clone --branch v2026.04.0 --depth 1 \
    https://github.com/OpenAMP/openamp-system-reference.git
```

检查：

```bash
cd openamp-system-reference
git describe --tags --always
```

期望看到：

```text
v2026.04.0
```

确认 manifest：

```bash
grep -A4 'name: zephyr' west.yml
```

应该看到：

```text
- name: zephyr
  remote: zephyr
  revision: v4.4.0
```

返回 workspace 根目录：

```bash
cd ~/work/openamp_ws
```

---

## 5. 用官方 manifest 拉取 Zephyr / OpenAMP / libmetal

`openamp-system-reference` 自己就是 west manifest repository，因此执行：

```bash
cd ~/work/openamp_ws
west init -l openamp-system-reference
west update
```

下载完成后目录应至少有：

```bash
ls
```

核心目录应包含：

```text
openamp-system-reference/
zephyr/
open-amp/
libmetal/
openamp-zephyr-modules/
modules/ ...
```

检查 west 认识的项目：

```bash
west list | grep -E 'zephyr|open-amp|libmetal|openamp-zephyr'
```

然后安装当前 workspace 对应的 Python requirements：

```bash
west packages pip --install
```

导出 Zephyr CMake package：

```bash
west zephyr-export
```

### 5.1 立即冻结实际拉取版本

因为 `v2026.04.0/west.yml` 中 OpenAMP/libmetal 仍使用 `main`：

```bash
west manifest --freeze -o west-frozen.yml
```

确认：

```bash
head -40 west-frozen.yml
```

此文件中各 project 的 revision 应已变成 SHA。

---

## 6. 安装 Zephyr SDK

进入 Zephyr：

```bash
cd ~/work/openamp_ws/zephyr
west sdk install
```

完成后再次进入 Demo build 时，CMake 应能报告类似：

```text
Found host-tools: zephyr ...
Found toolchain: zephyr ...
```

OpenAMP 项目升级 Zephyr 4.4 的公开构建记录中曾显示：

```text
Found host-tools: zephyr 1.0.1
Found toolchain: zephyr 1.0.1
```

### 6.1 查找 QEMU 和 `ivshmem-server`

不要硬编码旧 README 中的 SDK 路径，直接查：

```bash
find ~/zephyr-sdk-* -type f \
    \( -name 'qemu-system-aarch64' -o -name 'ivshmem-server' -o -name 'ivshmem-client' \) \
    -print
```

至少必须找到：

```text
qemu-system-aarch64
ivshmem-server
```

Zephyr SDK 的 QEMU recipe 会把 QEMU 的：

```text
contrib/ivshmem-server/ivshmem-server
contrib/ivshmem-client/ivshmem-client
```

安装到 SDK host-tools 的可执行目录，因此新版 SDK 中不要再依赖旧的 `usr/xilinx/bin/` 固定路径。

保存 `ivshmem-server` 路径：

```bash
export IVSHMEM_SERVER="$(find ~/zephyr-sdk-* -type f -name ivshmem-server -print -quit)"

echo "$IVSHMEM_SERVER"
test -x "$IVSHMEM_SERVER" && echo OK
```

如果输出 `OK`，继续。

---

## 7. 先检查 Demo 源码没有取错

```bash
cd ~/work/openamp_ws/openamp-system-reference/examples/zephyr/dual_qemu_ivshmem
find . -maxdepth 3 -type f | sort
```

至少应看到：

```text
./README.rst
./host/CMakeLists.txt
./host/prj.conf
./host/src/main.c
./remote/CMakeLists.txt
./remote/prj.conf
./remote/src/main.c
./rpmsg_ivshmem_backend/rpmsg_ivshmem_backend.c
./rpmsg_ivshmem_backend/rpmsg_ivshmem_backend.h
```

Host 配置：

```bash
cat host/prj.conf
```

重点确认：

```text
CONFIG_PCIE=y
CONFIG_PCIE_MSI=y
CONFIG_PCIE_MSI_X=y
CONFIG_PCIE_MSI_MULTI_VECTOR=y
CONFIG_VIRTUALIZATION=y
CONFIG_IVSHMEM=y
CONFIG_IVSHMEM_DOORBELL=y
CONFIG_OPENAMP=y
CONFIG_OPENAMP_SLAVE=n
```

Remote：

```bash
cat remote/prj.conf
```

重点确认末尾：

```text
CONFIG_OPENAMP=y
CONFIG_OPENAMP_MASTER=n
```

说明 Host / Remote 不是两套不同 OpenAMP，而是同一 backend 通过角色配置进入 RPMsg host/device 两侧。

---

## 8. 编译 Host

终端 A：

```bash
cd ~/work/openamp_ws
source .venv/bin/activate

cd openamp-system-reference/examples/zephyr/dual_qemu_ivshmem/host
west build -pauto -bqemu_cortex_a53
```

官方 Demo README 使用的就是：

```text
west build -pauto -bqemu_cortex_a53
```

第一次排错时如果怀疑旧 build cache，可改成：

```bash
west build -p always -b qemu_cortex_a53
```

构建成功后检查：

```bash
test -f build/zephyr/zephyr.elf && echo HOST_BUILD_OK
```

应输出：

```text
HOST_BUILD_OK
```

检查 Kconfig 最终值：

```bash
grep -E '^CONFIG_(OPENAMP|IVSHMEM|IVSHMEM_DOORBELL|PCIE_MSI)' \
    build/zephyr/.config
```

---

## 9. 编译 Remote

终端 B：

```bash
cd ~/work/openamp_ws
source .venv/bin/activate

cd openamp-system-reference/examples/zephyr/dual_qemu_ivshmem/remote
west build -pauto -bqemu_cortex_a53
```

检查：

```bash
test -f build/zephyr/zephyr.elf && echo REMOTE_BUILD_OK
```

期望：

```text
REMOTE_BUILD_OK
```

---

## 10. 启动 `ivshmem-server`

**运行顺序不能乱：**

```text
1. ivshmem-server
2. Host QEMU
3. Remote QEMU
```

终端 S：

```bash
export IVSHMEM_SERVER="$(find ~/zephyr-sdk-* -type f -name ivshmem-server -print -quit)"
sudo "$IVSHMEM_SERVER" -n 2
```

`-n 2` 来自官方 Demo README；此 Demo 的 Arm64 IVSHMEM doorbell 使用两个向量。

另开一个终端检查：

```bash
ls -l /dev/shm/ivshmem /tmp/ivshmem_socket
```

官方 README 给出的权限设置是：

```bash
sudo chgrp "$USER" /dev/shm/ivshmem
sudo chmod 060 /dev/shm/ivshmem
sudo chgrp "$USER" /tmp/ivshmem_socket
sudo chmod 060 /tmp/ivshmem_socket
```

如果你的 Linux 没有与用户名同名的 group，则改用：

```bash
id -gn
```

然后把 `chgrp "$USER"` 改成：

```bash
sudo chgrp "$(id -gn)" /dev/shm/ivshmem /tmp/ivshmem_socket
```

不要先随意改 QEMU 参数；先确保 socket 和 shared memory 文件存在且当前用户所属 group 有访问权限。

---

## 11. 启动 Host QEMU —— 必须先启动

终端 A：

```bash
cd ~/work/openamp_ws
source .venv/bin/activate
cd openamp-system-reference/examples/zephyr/dual_qemu_ivshmem/host

west build -t run
```

**不要先启动 Remote。**

Host backend 中 `CONFIG_OPENAMP_MASTER` 路径会等待 Remote 端 Name Service 建立 endpoint，所以此时 Host 可能停在等待状态，这是正常的。

---

## 12. 再启动 Remote QEMU

终端 B：

```bash
cd ~/work/openamp_ws
source .venv/bin/activate
cd openamp-system-reference/examples/zephyr/dual_qemu_ivshmem/remote

west build -t run
```

两端建立完成后，Host 应看到语义等价于：

```text
Host Side, the communication over RPMsg is ready to use!
```

Remote：

```text
Remote Side, the communication over RPMsg is ready to use!
```

注意：旧 README 示例中的 Zephyr boot version 字符串是 3.4.x；按本文 workspace 构建时应以你实际 Zephyr 4.4 build 的版本字符串为准，不要拿旧字符串做失败判断。

---

## 13. 发送第一组 RPMsg

回到 Host QEMU shell：

```text
uart:~$
```

先看帮助：

```text
rpmsg_ivshmem send
```

应该提示参数：

```text
rpmsg_ivshmem send <string> <number of messages>
```

发送 10 次：

```text
rpmsg_ivshmem send "RPMsg over IVSHMEM" 10
```

Host 侧应该连续收到十次 echo，例如：

```text
Remote side echoed the string back:
[ RPMsg over IVSHMEM ]
at message number 1
```

Remote 侧应该看到：

```text
Host side sent a string:
[ RPMsg over IVSHMEM ]
Now echoing it back!
```

### 13.1 最小验收标准

只有同时满足以下条件，才算 Demo 真正跑通：

```text
[PASS] Host build/zephyr/zephyr.elf 存在
[PASS] Remote build/zephyr/zephyr.elf 存在
[PASS] ivshmem-server 正在运行
[PASS] Host QEMU 先启动
[PASS] Remote QEMU 后启动
[PASS] Host 显示 RPMsg ready
[PASS] Remote 显示 RPMsg ready
[PASS] Host 发送 10 次消息
[PASS] Host 收到 10 次 echo
[PASS] Remote 收到并 echo 10 次
```

---

## 14. 做一次故障实验

这是官方 README 已设计好的行为验证。

Host 执行：

```text
rpmsg_ivshmem send "failure-test" 10
```

中途关闭 Remote QEMU。

Host 代码在：

```text
host/src/main.c
```

使用：

```c
k_sem_take(&rx_sem, K_MSEC(5000));
```

因此应在约 5 秒等待后出现：

```text
Remote side response timed out!
```

这个实验说明：应用层等待超时是 Host Demo 自己实现的，不是 `rpmsg_send()` 自动提供的“应用回复超时”。

### 14.1 官方已知限制

任意一侧 QEMU 被关闭后：

**不要只重启一侧。**

官方 README 明确要求：

```text
停止 Host
停止 Remote
必要时重启 ivshmem-server
然后重新：Host → Remote
```

原因不是 RPMsg 协议本身要求如此，而是这个 IVSHMEM Demo 的 peer ID/初始化模型限制。

---

# Part II：三天快速吃透 RPMsg 数据面源码

## 15. 源码仓库选择

只需要两个主仓库。

### Demo / 平台 glue

```text
OpenAMP/openamp-system-reference
```

固定入口：

```text
examples/zephyr/dual_qemu_ivshmem/
```

### OpenAMP 核心实现

```text
OpenAMP/open-amp
```

固定 release 阅读入口：

- `rpmsg.h`：<https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/include/openamp/rpmsg.h>
- `rpmsg.c`：<https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/rpmsg/rpmsg.c>
- `rpmsg_virtio.c`：<https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/rpmsg/rpmsg_virtio.c>
- `virtqueue.c`：<https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/virtio/virtqueue.c>
- `remoteproc.c`：<https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/remoteproc/remoteproc.c>
- `remoteproc_virtio.c`：<https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/remoteproc/remoteproc_virtio.c>

不要第一天先读 Linux kernel `drivers/rpmsg/`，也不要第一天读完 libmetal。

---

# Day 1：只看官方 Demo，建立“应用 → backend”模型

## 16. 第一步：读 Host `main.c`

路径：

```text
openamp-system-reference/
└── examples/zephyr/dual_qemu_ivshmem/
    └── host/src/main.c
```

只找四个东西。

### 16.1 获取 RPMsg device

源码：

```c
rpmsg_dev = get_rpmsg_ivshmem_device();
```

这里说明：

```text
应用层不知道 IVSHMEM 内存布局
应用层不知道 vring 地址
应用层不知道 doorbell

应用层只拿 rpmsg_device
```

这就是 OpenAMP 的层次边界。

### 16.2 创建 endpoint

Host 源码：

```c
rpmsg_create_ept(&ept,
                 rpmsg_dev,
                 "k",
                 RPMSG_ADDR_ANY,
                 get_rpmsg_ivshmem_ept_dest_addr(),
                 endpoint_cb,
                 rpmsg_service_unbind);
```

你先把它类比成：

```text
ipc-shm:
channel init + rx callback

OpenAMP:
rpmsg endpoint + local/dest address + callback
```

### 16.3 Host 发送

shell handler 中：

```c
rpmsg_send(&ept, str, strlen(str) + 1);
```

这就是你之后全部源码追踪的唯一入口。

### 16.4 Host 接收

```c
int endpoint_cb(struct rpmsg_endpoint *ept,
                void *data,
                size_t len,
                uint32_t src,
                void *priv)
```

收到远端 echo 后：

```c
k_sem_give(&rx_sem);
```

所以 Host 应用的状态机其实只有：

```text
shell command
    ↓
rpmsg_send()
    ↓
k_sem_take(5s)
    ↓
endpoint_cb()
    ↓
k_sem_give()
```

---

## 17. 第二步：读 Remote `main.c`

路径：

```text
remote/src/main.c
```

Remote endpoint：

```c
rpmsg_create_ept(&remote_ept,
                 rpmsg_dev,
                 "k",
                 RPMSG_ADDR_ANY,
                 RPMSG_ADDR_ANY,
                 endpoint_cb,
                 rpmsg_service_unbind);
```

收到 Host 数据后：

```c
rpmsg_send(&remote_ept, data, len);
```

因此整个 Demo 的业务逻辑完全可以压缩为：

```text
HOST                                  REMOTE

rpmsg_create_ept()                    rpmsg_create_ept()
       │                                     │
       │       rpmsg_send("hello")           │
       ├────────────────────────────────────►│
       │                                     │ endpoint_cb()
       │                                     │
       │        rpmsg_send(echo)              │
       │◄────────────────────────────────────┤
       │                                     │
 endpoint_cb()                               │
```

如果这张图还没完全理解，不要进 `virtqueue.c`。

---

## 18. 第三步：读最重要的 Demo 文件 `rpmsg_ivshmem_backend.c`

路径：

```text
rpmsg_ivshmem_backend/rpmsg_ivshmem_backend.c
```

这个文件就是“OpenAMP 平台 glue 的最小教材”。

### 18.1 共享内存

源码使用 Zephyr：

```c
shmem_size = ivshmem_get_mem(ivshmem_dev, &shmem_base);
```

然后构造 libmetal 的：

```c
struct metal_device shm_device
```

并完成：

```text
metal_init()
metal_register_generic_device()
metal_device_open()
metal_device_io_region()
```

对比 `ipc-shm`：

```text
ipc_os_get_local_shm()/ioremap()
          ↓
获得 CPU 可访问的共享内存
```

### 18.2 两个 VirtQueue

源码：

```c
vq[0] = virtqueue_allocate(VRING_SIZE);
vq[1] = virtqueue_allocate(VRING_SIZE);
```

Demo 常量：

```c
#define VRING_COUNT     2
#define VRING_ALIGNMENT 4
#define VRING_SIZE      16
```

所以当前实验不是一个神秘的大型框架，本质就是：

```text
2 个 vring / VirtQueue
每个 16 descriptors
```

### 18.3 RPMsg host / remote role

Host：

```c
vdev.role = RPMSG_HOST;
rpmsg_virtio_init_shm_pool(...);
rpmsg_init_vdev(..., ns_bind_cb, ..., &shpool);
```

Remote：

```c
vdev.role = RPMSG_REMOTE;
rpmsg_init_vdev(..., NULL, ..., NULL);
```

### 18.4 “发中断”的最终位置

Demo 的 VirtIO dispatch：

```c
struct virtio_dispatch dispatch = {
    ...
    .notify = virtio_notify,
};
```

`virtio_notify()` 最终：

```c
ivshmem_int_peer(ivshmem_dev, peer_dest_id, 0);
```

这就是 Demo 中最接近你熟悉的：

```c
ipc_hw_irq_notify();
```

的位置。

### 18.5 “收中断”的最终位置

IVSHMEM event thread：

```text
ivshmem_register_handler()
        ↓
k_poll(...)
        ↓
virtqueue_notification(vq[VIRTQUEUE_ID])
```

对比 ipc-shm：

```text
IRQ handler
    ↓
clear irq
    ↓
deferred RX
    ↓
ipc_shm_rx()
```

Day 1 验收：你必须能脱稿画出：

```text
rpmsg_send
   ↓
VirtQueue
   ↓
IVSHMEM
   ↓
doorbell
   ↓
virtqueue_notification
   ↓
endpoint_cb
```

---

# Day 2：从 `rpmsg_send()` 一直追到 vring

## 19. 第一站：`rpmsg.h`

路径：

```text
open-amp/lib/include/openamp/rpmsg.h
```

当前 `v2026.04.0` 中：

```c
static inline int rpmsg_send(struct rpmsg_endpoint *ept,
                             const void *data,
                             int len)
{
    ...
    return rpmsg_send_offchannel_raw(ept,
                                     ept->addr,
                                     ept->dest_addr,
                                     data,
                                     len,
                                     true);
}
```

第一层结论：

```text
rpmsg_send()
= 把 endpoint 的 local addr / dest addr
  带入通用发送函数
```

它本身没有碰共享内存。

---

## 20. 第二站：`rpmsg.c`

路径：

```text
open-amp/lib/rpmsg/rpmsg.c
```

重点只读：

```text
rpmsg_send_offchannel_raw()
rpmsg_create_ept()
rpmsg_get_endpoint()
rpmsg_send_ns_message()
```

当前实现的关键 dispatch：

```c
rdev = ept->rdev;

if (rdev->ops.send_offchannel_raw)
    return rdev->ops.send_offchannel_raw(...);
```

所以这一层类似 VFS 的 operations dispatch：

```text
RPMsg 通用层
   ↓
rpmsg_device.ops
   ↓
具体 transport 实现
```

你熟悉 Linux VFS 的话，可以直接把它理解为：

```text
read()
  ↓
file->f_op->read()
```

在这里对应：

```text
rpmsg_send_offchannel_raw()
  ↓
rdev->ops.send_offchannel_raw()
```

---

## 21. 第三站：`rpmsg_virtio.c`

路径：

```text
open-amp/lib/rpmsg/rpmsg_virtio.c
```

重点函数：

```text
rpmsg_virtio_send_offchannel_raw()
rpmsg_virtio_get_tx_buffer()
rpmsg_virtio_enqueue_buffer()
rpmsg_virtio_rx_callback()
rpmsg_init_vdev()
```

### 21.1 TX buffer 从哪里来

当前源码按 VirtIO role 分支：

```text
DRIVER role:
virtqueue_get_buffer()
或从 rpmsg shared-memory pool 取 buffer

DEVICE role:
virtqueue_get_first_avail_buffer()
```

这说明 OpenAMP 对 Host/Remote 两个角色使用同一个 RPMsg VirtIO transport，但 buffer 所有权方向不同。

### 21.2 Buffer 进入 VirtQueue

当前源码 `rpmsg_virtio_enqueue_buffer()` 中：

```text
DRIVER:
virtqueue_add_buffer(rvdev->svq, ...)

DEVICE:
virtqueue_add_consumed_buffer(rvdev->svq, ...)
```

随后发送路径最终调用：

```c
virtqueue_kick(rvdev->svq);
```

这就是 `ipc-shm` 中：

```text
push BD
    ↓
ipc_hw_irq_notify()
```

在 OpenAMP 中的对应分层：

```text
enqueue descriptor
    ↓
virtqueue_kick()
    ↓
VirtIO notify callback
    ↓
platform notify
```

---

## 22. 第四站：`virtqueue.c`

路径：

```text
open-amp/lib/virtio/virtqueue.c
```

这里只研究六类操作：

```text
创建 queue
加入 available buffer
获取 available buffer
加入 consumed/used buffer
获取 used buffer
kick/notification
```

重点搜索：

```bash
cd ~/work/openamp_ws/open-amp

grep -n 'virtqueue_kick' lib/virtio/virtqueue.c
grep -n 'virtqueue_add_buffer' lib/virtio/virtqueue.c
grep -n 'virtqueue_add_consumed_buffer' lib/virtio/virtqueue.c
grep -n 'virtqueue_get_first_avail_buffer' lib/virtio/virtqueue.c
grep -n 'virtqueue_get_buffer' lib/virtio/virtqueue.c
grep -n 'virtqueue_notification' lib/virtio/virtqueue.c
```

这一层要建立的心智模型只有：

```text
Producer

payload buffer
     ↓
descriptor[n]
     ↓
avail ring
     ↓
avail.idx++
     ↓
kick peer

--------------------------------

Consumer

notification
     ↓
看 avail.idx
     ↓
取 descriptor[n]
     ↓
处理 buffer
     ↓
used ring
     ↓
used.idx++
```

如果你熟悉 `ipc-shm`，这就是把自定义 BD/ring 标准化成 VirtIO vring。

---

## 23. Day 2 的完整 TX 调用链

按当前仓库代码应理解成：

```text
host/src/main.c
cmd_rpmsg_ivshmem_send()

        ↓

rpmsg_send()
lib/include/openamp/rpmsg.h

        ↓

rpmsg_send_offchannel_raw()
lib/rpmsg/rpmsg.c

        ↓

rdev->ops.send_offchannel_raw()

        ↓

rpmsg_virtio_send_offchannel_raw()
lib/rpmsg/rpmsg_virtio.c

        ↓

获取 TX buffer
填 RPMsg header + payload

        ↓

rpmsg_virtio_enqueue_buffer()

        ↓

VirtQueue / vring

        ↓

virtqueue_kick()

        ↓

Demo dispatch.notify

        ↓

rpmsg_ivshmem_backend.c::virtio_notify()

        ↓

ivshmem_int_peer()
```

这条链追通后，RPMsg 数据面已经掌握了一半以上。

---

## 24. Day 2 的完整 RX 调用链

Demo 另一侧：

```text
IVSHMEM doorbell

    ↓

ivshmem_event_loop_thread()

    ↓

virtqueue_notification()

    ↓

RPMsg VirtIO RX callback
rpmsg_virtio_rx_callback()

    ↓

读取 rpmsg_hdr

    ↓

根据 destination address 查 endpoint

    ↓

endpoint_cb()

    ↓

remote/src/main.c
```

对应 ipc-shm：

```text
IPI/IRQ
 ↓
ipc_hw_irq_clear
 ↓
ipc_shm_rx
 ↓
pop BD
 ↓
channel lookup
 ↓
rx_cb
```

---

# Day 3：再补 remoteproc，不把它和 RPMsg 混在一起

## 25. 为什么第三天才看 remoteproc

Dual-QEMU Demo 的重点是 RPMsg 数据面，它并没有演示 Linux remoteproc 把一个 R5 firmware 从 reset 状态加载并启动。

因此先明确两条正交路径：

```text
remoteproc：
firmware / memory / resource table / CPU start-stop

RPMsg：
endpoint / message / VirtIO / VirtQueue / notify
```

只有 Remote CPU 已经 running 后，RPMsg 数据面才有意义。

---

## 26. remoteproc 源码入口

先读接口：

```text
open-amp/lib/include/openamp/remoteproc.h
```

重点 API：

```text
remoteproc_init()
remoteproc_mmap()
remoteproc_set_rsc_table()
remoteproc_config()
remoteproc_load()
remoteproc_start()
remoteproc_stop()
remoteproc_shutdown()
```

当前头文件明确区分状态：

```text
RPROC_OFFLINE
RPROC_CONFIGURED
RPROC_READY
RPROC_RUNNING
RPROC_SUSPENDED
RPROC_ERROR
RPROC_STOPPED
```

再读实现：

```text
open-amp/lib/remoteproc/remoteproc.c
open-amp/lib/remoteproc/remoteproc_virtio.c
```

只回答四个问题：

```text
1. firmware 谁 load？
2. PA / DA / VA 如何映射？
3. resource table 如何变成 VirtIO/vring？
4. VirtQueue kick 最后如何进入 platform notify？
```

不要第一轮研究所有 resource type。

---

# Part III：源码动手实验

## 27. 实验 1：只观察应用层地址和长度

先不要改核心库。

在：

```text
host/src/main.c
remote/src/main.c
```

的 callback 中临时增加日志，观察：

```text
src
len
```

例如：

```c
printf("RPMsg RX: src=0x%x len=%zu\n", src, len);
```

重新编译 Host/Remote：

```bash
cd ~/work/openamp_ws/openamp-system-reference/examples/zephyr/dual_qemu_ivshmem/host
west build -pauto -bqemu_cortex_a53

cd ../remote
west build -pauto -bqemu_cortex_a53
```

重新按：

```text
ivshmem-server → Host → Remote
```

启动。

目标：确认 endpoint callback 收到的 `src` 来自 RPMsg transport header，而不是你的业务 payload。

---

## 28. 实验 2：观察 backend notify

在：

```text
rpmsg_ivshmem_backend/rpmsg_ivshmem_backend.c
```

找到：

```c
static void virtio_notify(struct virtqueue *vq)
```

这里官方源码已经存在：

```text
LOG_DBG("sending notification to the peer id ...")
```

如果默认看不到 debug 日志，**先不要改 OpenAMP core**；只在本 Demo 的 Kconfig/日志配置中提高 backend 日志级别，然后观察每次 `virtqueue_kick()` 是否最终引起 `ivshmem_int_peer()`。

实验问题：

```text
连续发送 10 条消息时：

应用 rpmsg_send 次数是多少？
notify 次数是多少？
remote callback 次数是多少？
```

这个实验对应你以前在 ipc-shm 中观察：

```text
push BD 次数
IRQ notify 次数
RX callback 次数
```

---

## 29. 实验 3：用 gdb/代码日志追 `virtqueue_kick`

源码定位：

```bash
cd ~/work/openamp_ws/open-amp

grep -Rns 'void virtqueue_kick' lib/
grep -Rns 'rpmsg_virtio_send_offchannel_raw' lib/
grep -Rns 'rpmsg_virtio_rx_callback' lib/
```

推荐第一轮**不用单步进入所有宏**，只设置逻辑断点：

```text
rpmsg_send_offchannel_raw
rpmsg_virtio_send_offchannel_raw
virtqueue_kick
virtio_notify   # Demo backend
rpmsg_virtio_rx_callback
endpoint_cb
```

观察变量：

```text
ept->addr
ept->dest_addr
src / dst / len
rvdev->svq
rvdev->rvq
vq->vq_queue_index
```

第二轮才继续下钻 descriptor / avail / used index。

---

# Part IV：你真正要记住的数据结构

## 30. `rpmsg_endpoint`

理解成：

```text
服务名
+ local address
+ destination address
+ RX callback
+ 所属 rpmsg_device
```

近似：

```text
ipc-shm channel + socket port
```

---

## 31. `rpmsg_device`

关键不是字段数量，而是：

```text
rpmsg_device
    │
    └── ops.send_offchannel_raw
```

RPMsg 通用层通过 ops 路由到具体 transport。

---

## 32. `rpmsg_virtio_device`

这是：

```text
RPMsg
 ↕
VirtIO/VirtQueue
```

之间的桥。

你只先关注：

```text
svq   send virtqueue
rvq   receive virtqueue
shpool
rdev
vdev
```

---

## 33. `virtqueue`

你要迁移成已有认知：

```text
virtqueue
≈
ipc-shm queue control block
```

真正的数据描述放在 vring descriptor / avail / used 结构中。

---

# Part V：和 NXP ipc-shm 的逐层映射

## 34. TX

```text
NXP ipc-shm

acquire buffer
   ↓
write payload
   ↓
push BD/ring
   ↓
ipc_hw_irq_notify()


OpenAMP

rpmsg_send()
   ↓
get TX buffer
   ↓
write RPMsg header + payload
   ↓
VirtQueue enqueue descriptor
   ↓
virtqueue_kick()
   ↓
platform notify / ivshmem_int_peer()
```

## 35. RX

```text
NXP ipc-shm

IRQ
 ↓
clear IRQ
 ↓
deferred RX
 ↓
pop BD
 ↓
channel lookup
 ↓
callback


OpenAMP Demo

IVSHMEM doorbell
 ↓
k_poll wakeup
 ↓
virtqueue_notification()
 ↓
RPMsg VirtIO RX
 ↓
RPMsg dst endpoint lookup
 ↓
endpoint_cb()
```

## 36. 底层适配

```text
ipc-shm
├── ipc-os
└── ipc-hw

OpenAMP
├── libmetal
│   ├── alloc
│   ├── cache
│   ├── io
│   ├── irq
│   ├── mutex
│   ├── shmem
│   ├── sleep
│   ├── time
│   └── atomic
│
└── platform / remoteproc ops
    ├── mmap
    ├── notify
    ├── start
    ├── stop
    └── shutdown
```

OpenAMP 官方 README 明确把上述 libmetal API 列为 porting OpenAMP 时需要实现/提供的系统接口。

---

# Part VI：常见故障与处理顺序

## 37. `west: command not found`

原因：没有进入 venv 或没有安装 west。

检查：

```bash
cd ~/work/openamp_ws
source .venv/bin/activate
which west
```

---

## 38. Python/CMake 版本过低

检查：

```bash
python3 --version
cmake --version
```

Zephyr 4.4 当前最低要求应满足：

```text
Python 3.12+
CMake 3.28+
```

老 Ubuntu 最省事的解决方案是升级主机环境，而不是在系统 Python 上混装依赖。

---

## 39. 找不到 `ivshmem-server`

不要先认定 SDK 没装。

```bash
find ~/zephyr-sdk-* -type f -name ivshmem-server -print
```

如果为空：

1. 确认 `west sdk install` 真正完成。
2. 确认安装的是包含 host-tools/QEMU 的完整 SDK。
3. 再考虑使用发行版预编译 `ivshmem-server` 或从 QEMU source 的 `contrib/ivshmem-server` 构建。

优先使用与 Zephyr SDK 配套的版本。

---

## 40. Host 一直没有 Ready

按以下顺序查：

```text
1. ivshmem-server 是否先运行？
2. /tmp/ivshmem_socket 是否存在？
3. Host 是否先于 Remote 启动？
4. Remote 是否真的启动？
5. 两边是否都使用各自 build 目录？
6. 是否只重启过其中一边？
```

如果只重启过一边：

```text
全部停掉
重新 ivshmem-server → Host → Remote
```

---

## 41. `Remote side response timed out!`

这不是证明 `rpmsg_send()` 一定失败。

Host Demo 逻辑是：

```text
rpmsg_send()
  ↓
等待 endpoint echo
  ↓
5 秒 k_sem timeout
```

所以要区分：

```text
发送失败
vs
发送成功但没有应用层 echo
```

---

## 42. Zephyr 4.4 与 README 的 3.4 示例输出不一致

不要用旧 boot banner 判断失败。

检查真正关键内容：

```text
Host Side ... RPMsg ... ready
Remote Side ... RPMsg ... ready
10 次 echo 是否闭环
```

---

# Part VII：源码分析的最终验收

你完成这份教程后，应能不看源码解释：

```text
1. Host `rpmsg_send()` 为什么不会直接操作 IVSHMEM？
2. `rpmsg_device.ops` 解决了什么抽象问题？
3. `rpmsg_virtio_device` 为什么同时有 send/receive virtqueue？
4. VirtQueue 和 ipc-shm ring 的本质对应关系是什么？
5. `virtqueue_kick()` 如何最终变成 IVSHMEM doorbell？
6. Remote doorbell 到 `endpoint_cb()` 的路径是什么？
7. endpoint address 和业务协议 command ID 为什么不是一回事？
8. remoteproc 为什么可以晚于 RPMsg 数据面再学？
9. 真正移植到新 SoC 时，IVSHMEM backend 要被什么替换？
10. libmetal 和 ipc-shm 的 OS/HW abstraction 分别如何对应？
```

如果这 10 个问题能脱稿讲清楚，就已经具备继续进入真实 SoC OpenAMP porting 的基础。

---

# Part VIII：执行报告

## 43. 本次云端执行环境

执行日期：2026-09-07 UTC

云端容器实际环境：

```text
OS: Debian GNU/Linux 13 (trixie)
Kernel: Linux 6.18.35 x86_64
Python: 3.13.5
CMake: 3.31.6
Git: 2.47.3
Ninja: installed
west: NOT_FOUND
qemu-system-aarch64: NOT_FOUND
ivshmem-server: NOT_FOUND
```

Python/CMake 版本满足 Zephyr 4.4 的最低工具版本要求。

## 44. 实际执行过的网络/依赖测试

实际执行：

```bash
git clone --depth 1 --branch v2026.04.0 \
    https://github.com/OpenAMP/openamp-system-reference.git
```

云端结果：

```text
fatal: unable to access 'https://github.com/OpenAMP/openamp-system-reference.git/':
Could not resolve host: github.com
```

实际执行：

```bash
python3 -m pip install west
```

结果：

```text
Temporary failure in name resolution
ERROR: No matching distribution found for west
```

实际执行：

```bash
apt-get update
```

结果：Debian mirror 无法建立外网连接，测试超时。

因此当前云端 sandbox 无法下载：

```text
west
Zephyr source tree
Zephyr SDK/QEMU
ivshmem-server
```

## 45. 动态 Demo 验证状态

**状态：BLOCKED，不是 PASS。**

当前云端无法诚实完成以下动态步骤：

```text
west update
west sdk install
Host build
Remote build
ivshmem-server -n 2
Host QEMU run
Remote QEMU run
10-message RPMsg echo
```

阻塞原因是 sandbox 网络/预装工具限制，而不是在 Demo 构建阶段发现了 OpenAMP 代码错误。

为了避免产生错误的工程结论，本文**没有把官方 README 的历史 expected output 冒充成本次云端实际输出**。

## 46. 已完成的源码级验证

虽然动态运行受阻，本次已经通过 OpenAMP/GitHub 官方源码接口逐项确认：

```text
[PASS] v2026.04.0 release 存在
[PASS] release 将 Zephyr 更新到 v4.4
[PASS] west.yml 明确 revision: v4.4.0
[PASS] dual_qemu_ivshmem Host/Remote/backend 文件存在
[PASS] 官方 README 构建命令是 west build -pauto -bqemu_cortex_a53
[PASS] 官方 README 要求 Host 先启动、Remote 后启动
[PASS] 官方测试 shell 命令是 rpmsg_ivshmem send <string> <count>
[PASS] Host main.c 实际调用 get_rpmsg_ivshmem_device → rpmsg_create_ept → rpmsg_send
[PASS] Host 使用 5 秒 semaphore 等待 echo
[PASS] Remote callback 实际调用 rpmsg_send() echo
[PASS] backend 实际使用 ivshmem_get_mem()
[PASS] backend 实际创建两个 VirtQueue
[PASS] backend 的 notify 实际调用 ivshmem_int_peer()
[PASS] backend RX event 实际调用 virtqueue_notification()
[PASS] OpenAMP v2026.04.0 rpmsg_send() 实际进入 rpmsg_send_offchannel_raw()
[PASS] rpmsg.c 实际通过 rdev->ops.send_offchannel_raw 做 transport dispatch
[PASS] rpmsg_virtio.c 实际使用 VirtQueue buffer API 并调用 virtqueue_kick()
[PASS] OpenAMP 主仓库当前健康检查 CI 在 Ubuntu 上成功完成 Zephyr build job
```

最后一项只能证明 OpenAMP/Zephyr 集成在 upstream CI 中仍被构建验证，**不能替代本次 Dual-QEMU 动态测试**。

## 47. 发现的坑及处理方案

### 坑 1：release 与 Demo README 的版本文字不一致

现象：

```text
release west.yml: Zephyr 4.4.0
README: tested with Zephyr 3.4.0 / SDK 0.16.1
```

处理：以 release manifest 为当前构建基线；保留 README 的启动/测试流程；使用 `west sdk install`；第一次成功更新后冻结 manifest。

### 坑 2：`open-amp` / `libmetal` 在 `v2026.04.0 west.yml` 仍指向 `main`

风险：今天和下个月 `west update` 可能拿到不同 SHA。

处理：

```bash
west manifest --freeze -o west-frozen.yml
```

### 坑 3：旧 README 给出的 `ivshmem-server` SDK 路径已经具有年代特征

处理：不要硬编码路径，使用：

```bash
find ~/zephyr-sdk-* -type f -name ivshmem-server -print
```

### 坑 4：Host/Remote 启动顺序是 Demo 的硬约束

处理：

```text
ivshmem-server → Host → Remote
```

任一 QEMU 异常退出后，两边一起重启。

### 坑 5：不要一开始读 remoteproc

原因：这个 Demo 的学习价值是数据面；remoteproc 不在这条 QEMU echo 主链中。

处理：

```text
Demo main.c
→ backend
→ rpmsg.h
→ rpmsg.c
→ rpmsg_virtio.c
→ virtqueue.c
→ 最后 remoteproc
```

---

# Part IX：参考源码 / 官方资料

1. OpenAMP System Reference release `v2026.04.0`  
   <https://github.com/OpenAMP/openamp-system-reference/releases/tag/v2026.04.0>

2. Dual-QEMU IVSHMEM RPMsg Demo  
   <https://github.com/OpenAMP/openamp-system-reference/tree/v2026.04.0/examples/zephyr/dual_qemu_ivshmem>

3. Demo README  
   <https://github.com/OpenAMP/openamp-system-reference/blob/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/README.rst>

4. Demo Host source  
   <https://github.com/OpenAMP/openamp-system-reference/blob/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/host/src/main.c>

5. Demo Remote source  
   <https://github.com/OpenAMP/openamp-system-reference/blob/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/remote/src/main.c>

6. Demo IVSHMEM backend  
   <https://github.com/OpenAMP/openamp-system-reference/blob/v2026.04.0/examples/zephyr/dual_qemu_ivshmem/rpmsg_ivshmem_backend/rpmsg_ivshmem_backend.c>

7. OpenAMP core `v2026.04.0`  
   <https://github.com/OpenAMP/open-amp/tree/v2026.04.0>

8. RPMsg public API  
   <https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/include/openamp/rpmsg.h>

9. RPMsg core  
   <https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/rpmsg/rpmsg.c>

10. RPMsg VirtIO transport  
    <https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/rpmsg/rpmsg_virtio.c>

11. VirtQueue  
    <https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/virtio/virtqueue.c>

12. RemoteProc API  
    <https://github.com/OpenAMP/open-amp/blob/v2026.04.0/lib/include/openamp/remoteproc.h>

13. Zephyr Getting Started  
    <https://docs.zephyrproject.org/latest/develop/getting_started/>

14. Zephyr IVSHMEM  
    <https://docs.zephyrproject.org/latest/services/virtualization/ivshmem.html>

15. Zephyr SDK  
    <https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html>

---

## 48. 下一阶段建议

真正完成本教程并在你的 Ubuntu 主机跑通 Dual-QEMU 后，再进入真实硬件：

```text
阶段 1：Dual-QEMU
RPMsg → VirtIO → VirtQueue → SHM → notify

阶段 2：真实 SoC
把 IVSHMEM backend 换成：
shared DDR/SRAM + Mailbox/IPI + cache/address mapping

阶段 3：remoteproc
firmware load + resource table + CPU lifecycle
```

不要反过来。
