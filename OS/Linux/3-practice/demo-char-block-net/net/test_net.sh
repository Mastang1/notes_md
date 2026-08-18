#!/bin/sh
# vnd0 测试：加载 → 配置 IP → ping 自己（发→回环→收）→ 看统计 → 卸载
# 用法: sh test_net.sh
set -e

sudo insmod vnd0.ko 2>/dev/null || true

sudo ip link set vnd0 up
sudo ip addr add 10.0.0.1/24 dev vnd0 2>/dev/null || \
	sudo ip addr replace 10.0.0.1/24 dev vnd0

echo "== ping 10.0.0.1（发→回环→收）=="
ping -c 3 10.0.0.1

echo "== 统计 =="
ip -s link show vnd0 | sed -n '1,6p'

echo "== cleanup =="
sudo ip link del vnd0 2>/dev/null || true
sudo rmmod vnd0
echo "vnd0 test OK ✅"
