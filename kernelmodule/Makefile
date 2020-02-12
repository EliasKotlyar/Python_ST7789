obj-m += st7789vw_fb.o olimexino_st7789vw.o

KBUILD=/lib/modules/$(shell uname -r)/build/
modules:
	make -C $(KBUILD) M=$(PWD) modules

modules_install:
	make -C $(KBUILD) M=$(PWD) modules_install

clean:
	make -C $(KBUILD) M=$(PWD) clean
