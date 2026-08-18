// test_misc.c — 用户态测试 /dev/misc_demo
// 编译: gcc -o test_misc test_misc.c
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
	char buf[128];
	int fd, n;

	fd = open("/dev/misc_demo", O_RDWR);
	if (fd < 0) { perror("open /dev/misc_demo"); return 1; }

	n = read(fd, buf, sizeof(buf));		/* 读默认内容 */
	if (n > 0) { write(1, "read: ", 6); write(1, buf, n); }

	write(fd, "hello from userspace", 20);	/* 写入 */

	lseek(fd, 0, SEEK_SET);			/* 回到文件头 */
	n = read(fd, buf, sizeof(buf));		/* 读回（应等于刚写的） */
	if (n > 0) { write(1, "after write: ", 13); write(1, buf, n); }

	close(fd);
	return 0;
}
