obj-m += dma_address_mapper.o

PWD := $(CURDIR)

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc um.c -o um ../go-pci-lib/libgopci.so -march=native

um: um.c
	gcc um.c -o um ../go-pci-lib/libgopci.so -march=native

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod dma_address_mapper || true
	rm -f um

insmod: all
	sudo insmod dma_address_mapper.ko

log:
	tail -f /var/log/messages

.PHONY: all clean insmod log um
