obj-m := second.o   
KDIR :=/home/shi/kernel/opt/EmbedSky/linux-2.6.30.4  
all:  
    make -C $(KDIR) M=$(shell pwd) modules  
test:  
    arm-linux-gcc -o second_test second_test.c  
install:  
    cp second.ko /mnt/hgfs/share/kernel/  
    cp second_test /mnt/hgfs/share/kernel/  
clean:  
    make -C $(KDIR) M=$(shell pwd) clean 
