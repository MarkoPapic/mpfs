KDIR = /lib/modules/`uname -r`/build

build:
	make -C $(KDIR) M=`pwd`

install:
	make -C $(KDIR) M=`pwd` INSTALL_MOD_PATH=`pwd` modules_install

mkfs_mpfs: mkfs_mpfs.o
	gcc -o mkfs_mpfs mkfs_mpfs.o -lm

mkfs_mpfs.o: mpfs.h

.PHONY: clean
clean:
	make -C $(KDIR) M=`pwd` clean
	rm -rf `pwd`/lib
	rm mkfs_mpfs