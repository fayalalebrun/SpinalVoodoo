KSRC ?= /usr/src/linux

obj-m += 3dfx.o

modules:
	ln -sf ../3dfx_driver.c 3dfx.c
	$(MAKE) -C $(KSRC) M=$(shell pwd) modules

clean:
	rm -f 3dfx.c
	$(MAKE) -C $(KSRC) M=$(shell pwd) clean

install: modules
	$(MAKE) -C $(KSRC) M=$(shell pwd) INSTALL_MOD_PATH="$(DESTDIR)" \
		modules_install

