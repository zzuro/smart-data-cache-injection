obj-m += iohub_ins.o
iohub_ins-y := iohub.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insmod: all
	sudo insmod iohub_ins.ko

log:
	tail -f /var/log/messages
