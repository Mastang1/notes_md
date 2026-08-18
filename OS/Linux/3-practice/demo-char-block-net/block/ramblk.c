// SPDX-License-Identifier: GPL-2.0
/*
 * ramblk.c — 最小块设备：4 MiB 内存当"磁盘"（blk-mq）
 *
 * 用法（在装有内核头文件的 Linux 上）：
 *   make && sudo insmod ramblk.ko
 *   sudo mkfs.ext4 /dev/ramblk
 *   sudo mount /dev/ramblk /mnt && echo hello > /mnt/f && sudo umount /mnt
 *   sudo rmmod ramblk
 *
 * 内核版本：面向 Linux 6.4+（blk_mq_alloc_disk/device_add_disk 新版签名）
 * 对应笔记：OS/Linux/0-user_space/用户态访问三分类-char-block-net.md 第 4 章
 */
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/vmalloc.h>

#define RAMBLK_SECTOR_SIZE	512
#define RAMBLK_SECTORS		(8 * 1024)	/* 4 MiB */
#define RAMBLK_QUEUE_DEPTH	128

static char *ram_data;			/* 内存"磁盘"本体 */
static struct gendisk *gd;
static struct blk_mq_tag_set tag_set;

/*
 * 块驱动的"搬运工"：上层把 request（一批 bio 段）派下来，
 * 我们照单把数据搬进/搬出 ram_data。
 */
static blk_status_t ramblk_queue_rq(struct blk_mq_hw_ctx *hctx,
				    const struct blk_mq_queue_data *bd)
{
	struct request *rq = bd->rq;
	struct bio_vec bvec;
	struct req_iterator iter;
	loff_t pos;

	blk_mq_start_request(rq);

	if (req_op(rq) == REQ_OP_FLUSH)		/* RAM 无需真刷盘 */
		goto done;

	pos = blk_rq_pos(rq) * RAMBLK_SECTOR_SIZE;	/* 扇区 → 字节偏移 */
	rq_for_each_segment(bvec, rq, iter) {		/* 遍历请求的每个内存段 */
		void *kaddr = kmap_local_page(bvec.bv_page) + bvec.bv_offset;
		if (rq_data_dir(rq) == READ)		/* 读：磁盘→内存 */
			memcpy(kaddr, ram_data + pos, bvec.bv_len);
		else					/* 写：内存→磁盘 */
			memcpy(ram_data + pos, kaddr, bvec.bv_len);
		kunmap_local(kaddr);
		pos += bvec.bv_len;
	}
done:
	blk_mq_end_request(rq, BLK_STS_OK);		/* 同步完成（简单驱动） */
	return BLK_STS_OK;
}

static const struct blk_mq_ops ramblk_mq_ops = {
	.queue_rq = ramblk_queue_rq,
};

/* 块设备几乎不需要回调（没有 read/write！数据走 queue_rq） */
static const struct block_device_operations ramblk_fops = {
	.owner = THIS_MODULE,
};

static int __init ramblk_init(void)
{
	int ret;

	ram_data = vmalloc(RAMBLK_SECTORS * RAMBLK_SECTOR_SIZE);
	if (!ram_data)
		return -ENOMEM;

	tag_set.ops = &ramblk_mq_ops;
	tag_set.nr_hw_queues = 1;
	tag_set.queue_depth = RAMBLK_QUEUE_DEPTH;
	tag_set.numa_node = NUMA_NO_NODE;
	tag_set.cmd_size = 0;
	tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
	tag_set.driver_data = NULL;
	ret = blk_mq_alloc_tag_set(&tag_set);
	if (ret)
		goto err_free_mem;

	/* Linux 6.4+ 签名: blk_mq_alloc_disk(set, queue_limits, queuedata) */
	gd = blk_mq_alloc_disk(&tag_set, NULL, NULL);
	if (IS_ERR(gd)) {
		ret = PTR_ERR(gd);
		goto err_free_tag;
	}

	gd->major = 0;				/* 动态主设备号 */
	gd->minors = 1;
	gd->fops = &ramblk_fops;
	snprintf(gd->disk_name, DISK_NAME_LEN, "ramblk");
	set_capacity(gd, RAMBLK_SECTORS);

	/* Linux 6.x 签名: device_add_disk(parent, disk, groups) */
	ret = device_add_disk(NULL, gd, NULL);
	if (ret)
		goto err_put_disk;

	pr_info("ramblk: 4MiB RAM disk ready as /dev/ramblk\n");
	return 0;

err_put_disk:
	put_disk(gd);
err_free_tag:
	blk_mq_free_tag_set(&tag_set);
err_free_mem:
	vfree(ram_data);
	return ret;
}

static void __exit ramblk_exit(void)
{
	del_gendisk(gd);
	put_disk(gd);
	blk_mq_free_tag_set(&tag_set);
	vfree(ram_data);
	pr_info("ramblk: removed\n");
}

module_init(ramblk_init);
module_exit(ramblk_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("minimal blk-mq RAM block device demo");
