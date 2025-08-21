obj-m += dma_address_mapper.o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insmod: all
	sudo insmod dma_address_mapper.ko

log:
	tail -f /var/log/messages
