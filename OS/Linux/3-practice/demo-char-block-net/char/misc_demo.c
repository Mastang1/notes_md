// SPDX-License-Identifier: GPL-2.0
/*
 * misc_demo.c — 最小字符设备（misc 方式）
 *
 * 用法：
 *   make && sudo insmod misc_demo.ko
 *   sudo chmod 666 /dev/misc_demo
 *   ./test_misc
 *   sudo rmmod misc_demo
 *
 * 设备节点：/dev/misc_demo（misc 框架 + devtmpfs 自动创建）
 * 对应笔记：OS/Linux/0-user_space/用户态访问三分类-char-block-net.md 第 3 章
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEMO_BUF_SIZE 64
static char demo_buf[DEMO_BUF_SIZE] = "hello from misc_demo\n";

static ssize_t demo_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	/* 内核帮你做 copy_to_user + 偏移管理 */
	return simple_read_from_buffer(buf, len, off, demo_buf, strlen(demo_buf));
}

static ssize_t demo_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	if (len >= DEMO_BUF_SIZE)
		len = DEMO_BUF_SIZE - 1;
	if (copy_from_user(demo_buf, buf, len))	/* 必须用 copy_from_user，不能直接 memcpy 用户指针 */
		return -EFAULT;
	demo_buf[len] = '\0';
	return len;
}

static const struct file_operations demo_fops = {
	.owner	= THIS_MODULE,
	.read	= demo_read,
	.write	= demo_write,
};

static struct miscdevice demo_misc = {
	.minor	= MISC_DYNAMIC_MINOR,	/* 动态次设备号（主设备号固定为 10） */
	.name	= "misc_demo",		/* → /dev/misc_demo */
	.fops	= &demo_fops,
};

static int __init demo_init(void)
{
	return misc_register(&demo_misc);
}

static void __exit demo_exit(void)
{
	misc_deregister(&demo_misc);
}

module_init(demo_init);
module_exit(demo_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("minimal misc char device demo");
