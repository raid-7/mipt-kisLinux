ccflags-y := -std=gnu99 -Wno-declaration-after-statement

obj-m += directory_kmodule.o
directory_kmodule-objs := kmodule.o directory.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
