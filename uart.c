#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/miscdevice.h>
#include <linuc/cdev.h>
#include <linux/fs.h>

#include <linux/ioport.h>
#include <asm/io.h>


//#define MYUART_MAJOR	255
#define MYUART_MINOR	0
#define MYUART_NAME	"myuart"

#define ADDR_BASE	0x7f005000
#define ULCON0		0x00
#define UCON0		0x04
#define UFCON0		0x08
#define UMCON0		0x0c
#define UTRSTAT0	0x10
#define UERSTAT0	0x14
#define UFSTAT0		0x18	/*fifo stat*/
#define UTXH0		0x20
#define URXH0		0x24	/*reveive buffer*/
#define UBRDIV0		0x28	/*set the board rate*/
#define UINTP0		0x30	//for intrrupt
#define UINTM0		0x38	//intrrupt mask

#define ADDR_SIZE	(ULCON0 - UINTM0)

unsigned long addr;

void
uart_pin_init(void)
{
	if (request_mem_region(ULCON0, ADDR_SIZE,
		"uart_addr") == NULL) {
		printk(KERN_ALERT, "mem_region error\n");
		return -ENOMEM;
	}

	addr = ioremap(ULCON0, ADDR_SIZE);

	iowrite32(0x03, addr + ULCON0);
	iowrite32(((0x1 << 2) | 0x1 | (2 <<10)), addr + UCON0);
	iowrite32(0, addr + UFCON0);
	iowrite32(0, addr + UMCON0);
	iowrite32(0, addr + UINTP0);
	iowrite32(0xf, addr + UINTM0);
}

void
myuart_open(struct inode *inode, struct file *filp)
{
	uart_pin_init();

}

void myuart_release()
{
	release_mem_region(ULCON0, ADDR_SIZE);
	iounmap(addr);
}

void 
myuart_read(struct file *filp, char __user *buffer,
		size_t count, loff_t offset)
{
	
}

void 
myuart_write(struct file *filp, char __user *buffer,
		size_t count, loff_t offset)
{

}

struct file_operations myuart_fops = {
	.owner		=	THIS_MODULE,
	.open		=	myuart_open,
	.read		=	myuart_read,
	.write		=	myuart_write,
	.unlocked_ioctl	=	myuart_ioctl,
	.release	=	myuart_release,
};

struct miscdevice *myuart_misc = {
	.minor = MYUART_MINOR,
	.name = MYUART_NAME,
	.fops = myuart_fops,
};

void __init myuart_init(void)
{
	misc_register(&myuart_misc);
}

void __exit myuart_exit(void)
{

}

module_init(myuart_init);
module_exit(myuart_exit);
MODULE_LICENSE("GPL");
