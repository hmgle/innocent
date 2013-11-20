KERNEL_RELEASE = $(shell uname -r)
KERNEL_VER_MAJOR = $(shell echo $(KERNEL_RELEASE) | cut -f1 -d.)
KERNEL_VER_MINOR = $(shell echo $(KERNEL_RELEASE) | cut -f2 -d.)
VER_GE_3_9 = $(shell [ $(KERNEL_VER_MAJOR) -gt 3 -o \( $(KERNEL_VER_MAJOR) -eq 3 -a $(KERNEL_VER_MINOR) -ge 9 \) ] && echo true)

ifeq ($(VER_GE_3_9),true)
$(shell echo "#define NEWKERN 1" > config.h)
else
$(shell echo "#define NEWKERN 0" > config.h)
endif

obj-m := innocent.o 

KDIR  := /lib/modules/$(shell uname -r)/build

PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	install -p idimo.txt /lib/modules/$(shell uname -r)/extra
	cp innocent.rules /etc/udev/rules.d/10_innocent.rules
	/sbin/depmod -a

clean:
	-rm -rf *.ko *.o .*.cmd config.h modules.order \
		Module.symvers .tmp_versions innocent.mod.c
