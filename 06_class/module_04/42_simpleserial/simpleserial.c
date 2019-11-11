#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/serial_core.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/arch/regs-serial.h>


#define portaddr(port, reg) ((port)->membase + (reg))
        
#define rd_regb(port, reg) (__raw_readb(portaddr(port, reg)))	
#define rd_regl(port, reg) (__raw_readl(portaddr(port, reg)))  

#define wr_regb(port, reg, val) \
  do { __raw_writeb(val, portaddr(port, reg)); } while(0)

#define wr_regl(port, reg, val) \
  do { __raw_writel(val, portaddr(port, reg)); } while(0)


#define SSER_MAJOR	126

char devicename[20];
static struct cdev my_cdev;
int sser_major;

struct uart_port *port;		

#define MAX_BUF	32

int sser_open(struct inode *inode, struct file *filp)
{
	port = kmalloc(sizeof(struct uart_port), GFP_ATOMIC);

	port->membase = (void __iomem *)S3C24XX_VA_UART0;	

	return 0;
}

ssize_t sser_read(struct file *filp, char *buf, size_t count, loff_t *l)
{
	char *b2;
	int i;

	b2 = kmalloc(count, GFP_ATOMIC);

	for (i=0; i<count; i++) {
		while( !(rd_regl(port, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_RXDR));
		b2[i] = rd_regb(port, S3C2410_URXH);
	}

	copy_to_user(buf, b2, count);

	kfree(b2);
	return count;
}

ssize_t sser_write(struct file *filp, const char *buf, size_t count, loff_t *l)
{
	char *b2;
	int i;

	b2 = kmalloc(count, GFP_ATOMIC);
	copy_from_user(b2, buf, count);

	for (i=0; i<count; i++) {
		while ( !(rd_regl(port, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXFE));
		wr_regb(port, S3C2410_UTXH, b2[i]);
	}
	
	kfree(b2);
	return count;
}

int sser_release(struct inode *inode, struct file *file)
{
	kfree(port);
	printk("sser_release called\n");
	return 0;
}

static struct file_operations sser_fops = {
	.open =  sser_open,
	.read =  sser_read,
	.write = sser_write,
	.release = sser_release,
};

static int __init simple_serial_init (void)
{
	dev_t dev;
	sser_major = SSER_MAJOR;
	dev = MKDEV(sser_major, 0);

	printk("starting simple_serial_init()...\n");
	strcpy(devicename, "Simple serial device driver");
	register_chrdev_region(dev, 128, devicename);
	cdev_init(&my_cdev, &sser_fops);
	cdev_add(&my_cdev, dev, 128);
	
	return 0;
}

static void __exit simple_serial_exit(void)
{
	printk("Closing simple serial driver\n");
	cdev_del(&my_cdev);
	unregister_chrdev_region(MKDEV(SSER_MAJOR,0),128);
}

module_init(simple_serial_init);
module_exit(simple_serial_exit);

MODULE_LICENSE("GPL");
