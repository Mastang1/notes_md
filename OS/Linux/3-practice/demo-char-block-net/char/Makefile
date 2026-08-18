# char 模块 Makefile（在装有内核头文件的 Linux 上执行 make）
obj-m += misc_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
