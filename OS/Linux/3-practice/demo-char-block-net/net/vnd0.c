// SPDX-License-Identifier: GPL-2.0
/*
 * vnd0.c — 最小虚拟网卡：发包"回环"成收包（loopback 式）
 *
 * 用法（在装有内核头文件的 Linux 上）：
 *   make && sudo insmod vnd0.ko
 *   sudo ip link set vnd0 up
 *   sudo ip addr add 10.0.0.1/24 dev vnd0
 *   ping 10.0.0.1        # ★ 包发出 → 回环 → 收到 → ping 通
 *   ip -s link           # 看 tx/rx 统计
 *   sudo rmmod vnd0
 *
 * 对应笔记：OS/Linux/0-user_space/用户态访问三分类-char-block-net.md 第 5 章
 */
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

static struct net_device *vnd_dev;

/*
 * 网卡驱动的核心回调：上层把 sk_buff（一个"包裹"）交给你，
 * 你负责"发出去"。这里没有真硬件，就把包重新投进协议栈，
 * 模拟"对端回了包"——于是本机 ping 自己会通。
 */
static netdev_tx_t vnd_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	skb->protocol = eth_type_trans(skb, dev);	/* 收包路径的标准预处理 */
	netif_rx(skb);					/* ★ 交给协议栈（模拟收到） */

	dev->stats.tx_packets++;
	dev->stats.rx_packets++;
	dev->stats.tx_bytes += skb->len;
	dev->stats.rx_bytes += skb->len;
	return NETDEV_TX_OK;				/* 驱动已接管 skb */
}

static int vnd_open(struct net_device *dev)
{
	netif_start_queue(dev);
	netif_carrier_on(dev);		/* 告诉内核"链路通了" */
	return 0;
}

static int vnd_stop(struct net_device *dev)
{
	netif_carrier_off(dev);
	netif_stop_queue(dev);
	return 0;
}

static const struct net_device_ops vnd_netdev_ops = {
	.ndo_open	= vnd_open,
	.ndo_stop	= vnd_stop,
	.ndo_start_xmit	= vnd_start_xmit,
};

static void vnd_setup(struct net_device *dev)
{
	ether_setup(dev);		/* 标准以太网默认值（MAC 类型、mtu=1500...） */
	dev->netdev_ops = &vnd_netdev_ops;
}

static int __init vnd_init(void)
{
	int ret;

	vnd_dev = alloc_netdev(0, "vnd0", NET_NAME_UNKNOWN, vnd_setup);
	if (!vnd_dev)
		return -ENOMEM;

	ret = register_netdev(vnd_dev);
	if (ret) {
		free_netdev(vnd_dev);
		return ret;
	}
	pr_info("vnd0: virtual netdev registered (loopback echo)\n");
	return 0;
}

static void __exit vnd_exit(void)
{
	unregister_netdev(vnd_dev);
	free_netdev(vnd_dev);
	pr_info("vnd0: removed\n");
}

module_init(vnd_init);
module_exit(vnd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("minimal virtual netdev (loopback echo) demo");
