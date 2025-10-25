obj-m += df_tmr.o
df_tmr-y := df.o smn.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insmod: all
	sudo insmod df_tmr.ko

log:
	tail -f /var/log/messages
