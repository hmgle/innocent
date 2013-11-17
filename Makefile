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
	-rm -rf *.ko *.o .*.cmd modules.order \
		Module.symvers .tmp_versions innocent.mod.c
