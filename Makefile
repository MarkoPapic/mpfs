KDIR = /lib/modules/`uname -r`/build

build:
	make -C $(KDIR) M=`pwd`

install:
	make -C $(KDIR) M=`pwd` INSTALL_MOD_PATH=`pwd` modules_install

clean:
	make -C $(KDIR) M=`pwd` clean
	rm -rf `pwd`/lib