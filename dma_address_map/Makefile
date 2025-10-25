obj-m += dma_address_mapper.o

PWD := $(CURDIR)

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod dma_address_mapper || true

insmod: all
	sudo insmod dma_address_mapper.ko

log:
	tail -f /var/log/messages

.PHONY: all clean insmod log 
