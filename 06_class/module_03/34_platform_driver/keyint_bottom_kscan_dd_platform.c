#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/regs-gpio.h>
#include <linux/workqueue.h> //work queue

#include <linux/cdev.h> //cdev_init
#include <linux/wait.h> //wait_event_interruptible
#include <asm/uaccess.h> //user access
#include <linux/fs.h> //file_operatios 

#include <linux/platform_device.h> //platform_driver_register()


#define DRV_NAME		"keyint"

#define	KEY_MATRIX_BASE1	0
#define	KEY_MATRIX_BASE2	2
#define	KEY_MATRIX_BASE3	4
#define	KEY_MATRIX_BASE4	6
#define	KEY_MATRIX_BASE5	8

#define EXAMPLE 00 
//======================================
// 100 : workqueue test
// 200 : tasklet test
// others : non bottom-half
//======================================

#if 1
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif

static int key_major = 0, key_minor = 0;
static int result;
static dev_t key_dev;
static struct cdev key_cdev;

static char dev_name[] = "rebis_keyscan";
static int cur_key, old_key;
static flag = 0;

static DECLARE_WAIT_QUEUE_HEAD(wq);

struct rebis_key_detection
{
    int             irq;
    int             pin;
    int             pin_setting;
    char            *name;
    int             last_state;
	#if (EXAMPLE == 100)
	struct work_struct gdetect;
	#elif (EXAMPLE == 200)
	struct tasklet_struct gdetect;
	#endif
};

static struct rebis_key_detection rebis_gd = {
    IRQ_EINT3, S3C2410_GPF3, S3C2410_GPF3_EINT3, "key-detect", 0
};

static int scan_input(void);
static int key_register_cdev(void);

static irqreturn_t
rebis_keyevent(int irq, void *dev_id, struct pt_regs *regs)
{
    struct rebis_key_detection *gd = (struct rebis_key_detection *) dev_id;
    int             state;

	state = 1;
	printk("gd= %x, keypad was pressed \n",gd);

    if (!gd)
        return IRQ_HANDLED;

	#if 1
    state = s3c2410_gpio_getpin(gd->pin);

    gd->last_state = state;

    gprintk("%s gd %s\n\n", gd->name, state ? "high" : "low");
	#endif

	#if (EXAMPLE == 100)
	schedule_work(&gd->gdetect);
	#elif (EXAMPLE == 200)
	tasklet_schedule(&gd->gdetect);
	#endif 

	flag = 1;
	wake_up_interruptible(&wq);

	return IRQ_HANDLED;

}

static int scan_input(void) {

	if(((readl(S3C2410_GPFDAT) >> 3) & 0x1) != 0x1)
	{
		if(!s3c2410_gpio_getpin(S3C2410_GPF3))
			return 2;
	}
	return 0;
}

#if (EXAMPLE == 100)
static void rebis_keyint_callback(void *pgd)
{
    struct rebis_key_detection *gd = (struct rebis_key_detection *)pgd;
    
    int state = gd->last_state;

	gprintk("workqueue callback call\n\n");
}
#elif (EXAMPLE == 200)
static void rebis_keyint_callback(ulong data)
{
    struct rebis_key_detection *gd = (struct rebis_key_detection *)data;
    
    int state = gd->last_state;

	//int key_base[5] = {KEY_MATRIX_BASE5, KEY_MATRIX_BASE4, KEY_MATRIX_BASE3, KEY_MATRIX_BASE2, KEY_MATRIX_BASE1};
	int i;

	gprintk("tasklet callback call\n");

	//for key scan
	#if 0
	cur_key = 0;
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_INP);
	for(i=4; i>=0; i--)
	{
		writel(readl(S3C2410_GPBDAT) | (0x1f), S3C2410_GPBDAT);
		writel(readl(S3C2410_GPBDAT) & (~(0x1 << i)), S3C2410_GPBDAT);
		
		if(cur_key = scan_input())
		//cur_key = scan_input();
		{
			cur_key += (4-i);//key_base[i];
			if(cur_key == old_key)
				return 0;
			old_key = cur_key;
			printk("cur_key = %d \n\n", cur_key);
			//put_user(cur_key,(char *)buff);
			break;
		}
	}
	old_key = 0;

	// set GPBDAT 0
	s3c2410_gpio_setpin(S3C2410_GPB0, 0);
	s3c2410_gpio_setpin(S3C2410_GPB1, 0);
	s3c2410_gpio_setpin(S3C2410_GPB2, 0);
	s3c2410_gpio_setpin(S3C2410_GPB3, 0);
	s3c2410_gpio_setpin(S3C2410_GPB4, 0);
	
	// change External Interrupts
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_EINT3);
	#endif
}
#endif

static ssize_t rebis_keyscan_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	int i;
	int key_base[5] = {KEY_MATRIX_BASE1, KEY_MATRIX_BASE2, KEY_MATRIX_BASE3, KEY_MATRIX_BASE4, KEY_MATRIX_BASE5};

	cur_key = 0;
	#if 0
	interruptible_sleep_on(&wq);
	#else
	wait_event_interruptible(wq, flag != 0);
	//wait_event_interruptible_timeout(wq, flag != 0, 600);
	#endif
#if	KEY_DEBUG
	printk("key input\n");
#endif

	// change input port
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_INP);
	s3c2410_gpio_cfgpin(S3C2410_GPF4, S3C2410_GPF4_INP);

	for(i=4; i>=0; i--)
	{
		writel(readl(S3C2410_GPBDAT) | (0x1f), S3C2410_GPBDAT);
		writel(readl(S3C2410_GPBDAT) & (~(0x1 << i)), S3C2410_GPBDAT);
		
		if(cur_key = scan_input())
		{
			cur_key += key_base[i];
			if(cur_key == old_key)
				return 0;
			old_key = cur_key;
			put_user(cur_key,(char *)buff);
			break;
		}
	}
	
	old_key = 0;	
	flag = 0;

	// set GPBDAT 0
	s3c2410_gpio_setpin(S3C2410_GPB0, 0);
	s3c2410_gpio_setpin(S3C2410_GPB1, 0);
	s3c2410_gpio_setpin(S3C2410_GPB2, 0);
	s3c2410_gpio_setpin(S3C2410_GPB3, 0);
	s3c2410_gpio_setpin(S3C2410_GPB4, 0);
	
	// change External Interrupts
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_EINT3);


#if	KEY_DEBUG
	printk("Read Function\n");	
	printk("GPBDAT = 0x%08x\n", readl(S3C2410_GPBDAT));
	printk("GPFCON = 0x%08x\n", readl(S3C2410_GPFCON));
	printk("EXTINT0 = 0x%08x\n", readl(S3C2410_EXTINT0));
#endif

	return count;
}

static int rebis_keyscan_open(struct inode * inode, struct file * file)
{
	old_key = 0;

	printk(KERN_INFO "ready to scan key value\n");

	return 0;
}

static int rebis_keyscan_release(struct inode * inode, struct file * file)
{
	printk(KERN_INFO "end of the scanning\n");

	return 0;
}

static struct file_operations rebis_keyscan_fops = {
	.owner		= THIS_MODULE,
	.open		= rebis_keyscan_open,
	.release	= rebis_keyscan_release,
	.read		= rebis_keyscan_read,
};

static int s3c2410keypad_remove(struct platform_device *pdev)
{
	free_irq(rebis_gd.irq, &rebis_gd);
		
	#if (EXAMPLE == 100)
	#elif (EXAMPLE == 200)
	tasklet_kill(&rebis_gd.gdetect);
	#endif

	printk("this is s3c2410keypad_remove\n");
	return 0;
}

static int __init s3c2410keypad_probe(struct platform_device *pdev)
{
	int ret;
	

		// set output mode
	s3c2410_gpio_cfgpin(S3C2410_GPB0, S3C2410_GPB0_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB2, S3C2410_GPB2_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB3, S3C2410_GPB3_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB4, S3C2410_GPB4_OUTP);
	// set data
	s3c2410_gpio_setpin(S3C2410_GPB0, 0);
	s3c2410_gpio_setpin(S3C2410_GPB1, 0);
	s3c2410_gpio_setpin(S3C2410_GPB2, 0);
	s3c2410_gpio_setpin(S3C2410_GPB3, 0);
	s3c2410_gpio_setpin(S3C2410_GPB4, 0);

	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_EINT3);
	writel(readl(S3C2410_EXTINT0) & (~(0xf << 12)), S3C2410_EXTINT0);	
	writel(readl(S3C2410_EXTINT0) | (0x2 << 12), S3C2410_EXTINT0); // Falling Edge interrupt
	

	if( request_irq(IRQ_EINT3, rebis_keyevent, SA_INTERRUPT, DRV_NAME, &rebis_gd) )     
    {
                printk("failed to request external interrupt.\n");
                ret = -ENOENT;
               return ret;
    }
	#if (EXAMPLE == 100)
	INIT_WORK(&rebis_gd.gdetect, rebis_keyint_callback, &rebis_gd);
	#elif (EXAMPLE == 200)
	tasklet_init(&rebis_gd.gdetect, rebis_keyint_callback, (unsigned long)(&rebis_gd)); 
	#endif

    printk("this is s3c2410keypad_probe\n");

	
	return 0;
}



static struct platform_driver s3c2410keypad_driver = {
       .driver         = {
	       .name   = "s3c2410-keypad",
	       .owner  = THIS_MODULE,
       },
       .probe          = s3c2410keypad_probe,
       .remove         = s3c2410keypad_remove,
};

static int __init rebis_keyint_init(void)
{

    key_register_cdev();

	result = platform_driver_register(&s3c2410keypad_driver);

	printk(KERN_INFO "%s successfully loaded\n", DRV_NAME);

	return result;
    
}

static void __exit rebis_keyint_exit(void)
{
    
	cdev_del(&key_cdev);
	unregister_chrdev_region(key_dev, 1);

	platform_driver_unregister(&s3c2410keypad_driver);

    printk(KERN_INFO "%s successfully removed\n", DRV_NAME);
}
#if 1
static int key_register_cdev(void)
{
    int error;
	
	/* allocation device number */
	if(key_major) {
		key_dev = MKDEV(key_major, key_minor);
		error = register_chrdev_region(key_dev, 1, dev_name);
	} else {
		error = alloc_chrdev_region(&key_dev, key_minor, 1, dev_name);
		key_major = MAJOR(key_dev);
	}
	
	if(error < 0) {
		printk(KERN_WARNING "keyscan: can't get major %d\n", key_major);
		return result;
	}
	printk("major number=%d\n", key_major);
	
	/* register chrdev */
	cdev_init(&key_cdev, &rebis_keyscan_fops);
	key_cdev.owner = THIS_MODULE;
	error = cdev_add(&key_cdev, key_dev, 1);
	
	if(error)
		printk(KERN_NOTICE "Keyscan Register Error %d\n", error);
	
	return 0;
}
#endif

module_init(rebis_keyint_init);
module_exit(rebis_keyint_exit);

MODULE_AUTHOR("Jurngyu,Park <jurngyu@mdstec.com>");
MODULE_LICENSE("GPL");

/********************************************************
* platform device µî·Ï °úÁ¤ 
M2 board : arch/arm/mach-s3c2440/mach-mep2440.c

static struct platform_device *mep2440_devices[] __initdata = {
    &s3c_device_usb,
    &s3c_device_lcd,
    &s3c_device_wdt,
    &s3c_device_i2c,
    &s3c_device_iis,
    &mep2440_dm9k0,
    &s3c_device_nand,
	&s3c_device_keypad, --> Ãß°¡

};


 arch/arm/plat-s3c24xx/devs.c
struct platform_device s3c_device_keypad = {
    .name   = "s3c2410-keypad",
    .id     = -1,
};
EXPORT_SYMBOL(s3c_device_keypad);

include/asm-arm/plat-s3c24xx/devs.h
extern struct platform_device s3c_device_keypad; 

**************************************************************/
