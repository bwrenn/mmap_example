# Change these to reflect the architecture and compiler for your sustem, or comment
# them out if you don't want to cross compile.
export ARCH := arm64
export CROSS_COMPILE := aarch64-linux-gnu-

obj-m := mmap_test_kernel.o 

# Change KDIR to point to the location with your kernel source if you are cross
# compiling.  If you aren't cross compiling, replace KDIR with this commented line.
# You may need to pull down the kernel source with something like
# "sudo apt-get source linux-source"
#KDIR := /lib/modules/`uname -r`/build
KDIR := /home/bwrenn/SD410/debian-rt/Linaro-4.4.9/kernel

PWD := $(shell pwd)

default: kern user

kern:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.o modules.order Module.symvers *.ko *.mod.c *.mod.o mmap_test_user

user:
	$(CROSS_COMPILE)gcc mmap_test_user.c -o mmap_test_user
