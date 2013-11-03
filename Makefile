obj-m := innocent.o 

KDIR  := /lib/modules/$(shell uname -r)/build

PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	-rm -rf *.ko *.o .*.cmd modules.order \
		Module.symvers .tmp_versions innocent.mod.c
