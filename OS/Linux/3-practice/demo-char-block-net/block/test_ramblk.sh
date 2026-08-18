#!/bin/sh
# ramblk 测试：加载 → 建文件系统 → 挂载 → 读写 → 卸载
# 用法: sh test_ramblk.sh
set -e

sudo insmod ramblk.ko 2>/dev/null || true

echo "== lsblk =="
lsblk | grep ramblk || true

echo "== mkfs.ext4 =="
sudo mkfs.ext4 -q /dev/ramblk

echo "== mount + write/read =="
sudo mkdir -p /mnt/ramblk
sudo mount /dev/ramblk /mnt/ramblk
echo "hello from ramblk" | sudo tee /mnt/ramblk/hello
sudo cat /mnt/ramblk/hello
sudo umount /mnt/ramblk

echo "== cleanup =="
sudo rmmod ramblk
echo "ramblk test OK ✅"
