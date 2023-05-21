#KERN_DIR = /home/book/100ask_imx6ull-sdk/Linux-4.9.88
#KERN_DIR = /usr/src/linux-headers-5.4.0-148-generic
KERN_DIR = /usr/src/linux-headers-4.15.0-39-generic
#把编译好的ko文件加载模块时出错：Error: could not insert module hello_world.ko: Invalid module format
#虚拟机内核版本uname-a查看
#选择源码路径
#4.9.88是开发板上的路径
all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
# 参考内核源码drivers/char/ipmi/Makefile
# 要想把a.c, b.c编译成ab.ko, 可以这样指定:
# ab-y := a.o b.o
# obj-m += ab.o

obj-m += myvivid.o