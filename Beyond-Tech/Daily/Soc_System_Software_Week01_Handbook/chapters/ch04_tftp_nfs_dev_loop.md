# Chapter 4 - 建立 TFTP + NFS：把一次修改缩短成几十秒

## 4.1 本章目标

今天把：

```text
修改源码 -> 编译 -> 烧卡 -> 重启
```

改成：

```text
Laptop edit
 -> Ubuntu Host compile
 -> TFTP/NFS
 -> Target test
```

正点原子资料对两者的定位：
- TFTP：U-Boot/网络下载；
- NFS：Linux 挂共享目录/根文件系统；
- 驱动指南 4.2.1 把 NFS 作为后续开发环境。

---

## 4.2 先确认基础网络

Host：

```bash
ping -c 3 <target-ip>
```

Target：

```bash
ping -c 3 <host-ip>
```

失败就回 Chapter 3。

以下示例：

```text
HOST=192.168.10.10
TARGET=192.168.10.20
```

---

## 4.3 安装 TFTP

```bash
sudo apt update
sudo apt install -y tftpd-hpa tftp-hpa
```

创建 root：

```bash
sudo mkdir -p /srv/tftp
sudo chown -R tftp:tftp /srv/tftp
sudo chmod 755 /srv/tftp
```

测试文件：

```bash
echo "week1-tftp-ok" | sudo tee /srv/tftp/health.txt
sudo chmod 644 /srv/tftp/health.txt
```

---

## 4.4 配置 `tftpd-hpa`

备份：

```bash
sudo cp /etc/default/tftpd-hpa \
  /etc/default/tftpd-hpa.backup.$(date +%Y%m%d_%H%M%S)
```

编辑：

```bash
sudo nano /etc/default/tftpd-hpa
```

建议：

```text
TFTP_USERNAME="tftp"
TFTP_DIRECTORY="/srv/tftp"
TFTP_ADDRESS=":69"
TFTP_OPTIONS="--secure"
```

重启：

```bash
sudo systemctl restart tftpd-hpa
systemctl status tftpd-hpa --no-pager
sudo ss -lunp | grep ':69'
```

---

## 4.5 先做 Host 本机 TFTP 自测

```bash
cd /tmp
rm -f health.txt
tftp 127.0.0.1
```

交互：

```text
tftp> get health.txt
tftp> quit
```

检查：

```bash
cat /tmp/health.txt
```

必须：

```text
week1-tftp-ok
```

本机失败时，绝对不要去 U-Boot 里反复试。

---

## 4.6 U-Boot：先看环境

```bash
printenv ipaddr
printenv serverip
printenv netmask
printenv gatewayip
printenv ethaddr
printenv loadaddr
```

临时设置：

```bash
setenv ipaddr 192.168.10.20
setenv serverip 192.168.10.10
setenv netmask 255.255.255.0
```

先：

```bash
ping ${serverip}
```

成功才继续。

### 为什么不立即 `saveenv`

先让修改保持在 RAM。IP 写错后 reset 能恢复。

---

## 4.7 U-Boot TFTP 下载小文件

先：

```bash
printenv loadaddr
```

如果 `loadaddr` 没定义，不要抄别人的 DDR 地址。需要根据当前 U-Boot/`bdinfo` 确认有效 RAM 地址。

若有：

```bash
tftp ${loadaddr} health.txt
```

成功应有：

```text
Bytes transferred = ...
```

查看：

```bash
md.b ${loadaddr} 0x20
```

证明：

```text
/srv/tftp/health.txt
 -> tftpd
 -> Ethernet
 -> U-Boot
 -> DRAM
```

---

## 4.8 TFTP 失败定位

### ping 都失败

回 Chapter 3。

### ping 成功，TFTP timeout

Host：

```bash
systemctl status tftpd-hpa
sudo ss -lunp | grep ':69'
sudo tcpdump -ni <host-iface> udp port 69
```

### File not found

```bash
ls -l /srv/tftp/health.txt
```

U-Boot 请求 `health.txt`，不是 `/srv/tftp/health.txt`。

---

## 4.9 安装 NFS Server

```bash
sudo apt install -y nfs-kernel-server
sudo mkdir -p /srv/nfs/imx6ull
sudo chown -R "$USER":"$USER" /srv/nfs/imx6ull
```

编辑：

```bash
sudo nano /etc/exports
```

示例网段：

```exports
/srv/nfs/imx6ull 192.168.10.0/24(rw,sync,no_subtree_check,no_root_squash)
```

参数：
- `rw`：读写；
- `sync`：同步提交；
- `no_subtree_check`：避免 subtree 检查；
- `no_root_squash`：开发板 root 保持 root，仅用于可信开发网。

应用：

```bash
sudo exportfs -rav
sudo systemctl restart nfs-kernel-server
showmount -e localhost
```

---

## 4.10 Target 挂 NFS

先：

```bash
cat /proc/filesystems | grep nfs
mkdir -p /mnt/nfs
```

老嵌入式 BSP 常用：

```bash
mount -t nfs -o nolock,vers=3 \
  192.168.10.10:/srv/nfs/imx6ull \
  /mnt/nfs
```

检查：

```bash
mount | grep nfs
df -h /mnt/nfs
```

若版本错误：

Host：

```bash
rpcinfo -p localhost | grep nfs
```

不要随机堆 mount 参数，要匹配 client/server 能力。

---

## 4.11 第一个真正的 Host -> Target loop

Host：

```bash
cd ~/work/linux/apps/week1_elf
cp hello_arm_static /srv/nfs/imx6ull/
sync
```

Target：

```bash
cd /mnt/nfs
ls -l
chmod +x hello_arm_static
./hello_arm_static
```

预期：

```text
hello from week1
```

在 Laptop 的 VS Code Remote SSH 修改 `hello.c` 字符串。

Host：

```bash
arm-linux-gnueabihf-gcc -static -Wall -Wextra \
  ~/work/linux/apps/week1_elf/hello.c \
  -o /srv/nfs/imx6ull/hello_arm_static
```

Target：

```bash
/mnt/nfs/hello_arm_static
```

不用 SCP，不用拔卡，不用烧写。

---

## 4.12 为什么这里使用 static

动态 `hello_arm` 依赖 Target rootfs 的 dynamic loader/libc。

Ubuntu 24.04 cross sysroot 与老出厂 rootfs 可能不完全兼容。

所以本实验先只验证：

```text
source -> compile -> share -> execute
```

这是控制变量，不是逃避动态链接。

---

## 4.13 NFS 故障树

### Connection refused

```bash
systemctl status nfs-kernel-server
showmount -e localhost
```

### access denied

```bash
sudo exportfs -v
```

确认 Target IP 在允许网段。

### Protocol not supported

Target：

```bash
cat /proc/filesystems | grep nfs
```

Host：

```bash
rpcinfo -p localhost | grep nfs
```

### mount 成功但文件不对

Host：

```bash
ls -la /srv/nfs/imx6ull
```

Target：

```bash
mount | grep /mnt/nfs
ls -la /mnt/nfs
```

---

## 4.14 本章验收

从零完成：

```text
Host health.txt -> U-Boot TFTP -> DRAM
Host hello_arm_static -> NFS -> Target execute
```

口述：
1. `serverip` 是 Host 还是 Target？
2. TFTP 为什么适合 U-Boot？
3. NFS 为什么适合 Linux 开发阶段？
4. 为什么本实验先 static？

---

## 4.15 原始资料

- `ALI-NET-1.3.1`：《I.MX6U 网络环境 TFTP&NFS 搭建手册 V1.3.1》：[PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E7%BD%91%E7%BB%9C%E7%8E%AF%E5%A2%83TFTP%26NFS%E6%90%AD%E5%BB%BA%E6%89%8B%E5%86%8CV1.3.1.pdf)
- `ALI-DRV-1.5.2`：第4章 **4.2.1 NFS 服务开启**：[PDF](https://github.com/alientek-openedv/imx6ull-document/blob/master/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91I.MX6U%E5%B5%8C%E5%85%A5%E5%BC%8FLinux%E9%A9%B1%E5%8A%A8%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97V1.5.2.pdf)
- 正点原子公开仓库说明明确把该网络手册用于内核调试和文件系统挂载。
