obj-m += slave_err_ins.o
slave_err_ins-y := slave_err.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insmod: all
	sudo insmod slave_err_ins.ko

log:
	tail -f /var/log/messages
