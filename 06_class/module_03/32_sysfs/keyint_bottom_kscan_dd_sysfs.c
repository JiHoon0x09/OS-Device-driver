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
static dev_t key_dev = 0;
static struct cdev key_cdev;

static char dev_name[] = "rebis_keyscan";
static int cur_key, old_key;
static flag = 0;

static DECLARE_WAIT_QUEUE_HEAD(wq);

struct ldd_device{
	char			*name;
	unsigned short	vendor;
	struct device	dev;
};
#define to_ldd_device(dev) container_of(dev, struct ldd_device, dev);

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

#define to_ldd_device(dev) container_of(dev, struct ldd_device, dev);

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

/*** bus registeration ****/
///*

//*/
#if 0
static int ldd_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev->bus_id, driver->name, strlen(driver->name));
}
#endif

struct bus_type ldd_bus_type = {
	.name = "ldd",
//	.match = ldd_match,
//	.hotplug = ldd_hotplug,
};

static void pdev_release(struct device * dev){
//	dev->parent 	= NULL;
	printk("pdev_release done\n");
}
//static void release_pdev(struct device * dev){
static void ldd_bus_release(struct device * dev){
	printk(KERN_DEBUG "ldd_bus release\n");
}
struct device ldd_bus = {
	.bus_id = "ldd0",
	.release	= ldd_bus_release,

};
#if 1 //platform_device
static struct ldd_device pdev  =
{
	.name = "s3c2410-keypad",
	.vendor = 100,
	.dev	= {
//		.bus		= &ldd_bus_type,
//		.parent		= &ldd_bus, 
//		.bus_id		= "s3c2410-keypad",
//		.release	= pdev_release,
	},
};
#else


static struct ldd_device *ldddev = {
	.name = "s3c2410-keypad",
	.dev = ldd_bus,
};

/*
static struct device pdev  =
{
	.bus_id	= "kkk",
	.bus	= &platform_bus_type,
	.release	= release_pdev,	
};
*/
#endif

#if 0 //platform_driver
static struct platform_driver s3c2410keypad_driver = {
       .driver         = {
	       .name   = "s3c2410-keypad",
	       .owner  = THIS_MODULE,
       },
       .probe          = s3c2410keypad_probe,
       .remove         = s3c2410keypad_remove,
};
#else //device_driver
static struct device_driver s3c2410keypad_driver = {
	   .name   = "s3c2410-keypad",
	   .owner  = THIS_MODULE,
	  // .bus		= &platform_bus_type,
	   .bus			= &ldd_bus_type,
       .probe          = s3c2410keypad_probe,
       .remove         = s3c2410keypad_remove,
};
#endif


static void key_class_release(struct class_device *dev)
{
	printk("key class release\n"); 
}

static struct class key_class = {
	.name	="key",
	.release = key_class_release,
};
//static struct class_device cdevice_key;


//static struct class_device key_cdevice;
static struct class_device key_cdevice = {
	.class = &key_class,
//	.devt = key_dev,
	.class_id	= "keypad0",
};

int ldd_device_register(struct ldd_device *ldddev)
{
	ldddev->dev.bus = &ldd_bus_type;
	ldddev->dev.parent = &ldd_bus;
	ldddev->dev.release = pdev_release;
	strncpy(ldddev->dev.bus_id, ldddev->name, BUS_ID_SIZE);
	return device_register(&ldddev->dev);
}
EXPORT_SYMBOL(ldd_device_register);

void ldd_device_unregister(struct ldd_device *ldddev)
{
	device_unregister(&ldddev->dev);
}
EXPORT_SYMBOL(ldd_device_unregister);


static int __init rebis_keyint_init(void)
{
	//key_class = class_create(THIS_MODULE, "key");
	//if(IS_ERR(key_class))
	//	return PTR_ERR(key_class);
	
	key_register_cdev();

	/*** bus register ***/
	result = bus_register(&ldd_bus_type);
	if(result)
		return result;
	

	//class_device_add(&cdevice_key);
	class_register(&key_class);
///*
//	memset(&key_cdevice, 0, sizeof(key_cdevice));
//	key_cdevice.class = &key_class;
	key_cdevice.devt = key_dev;
	//key_cdevice.class_id = "key0";
//	strlcpy(key_cdevice.class_id, "keypad0", KOBJ_NAME_LEN);
//	printk("DBG devt = 0x%x \n", key_cdevice.devt);
//*/
	class_device_register(&key_cdevice);

	
	//result = platform_driver_register(&s3c2410keypad_driver);
	result = driver_register(&s3c2410keypad_driver);
	#if 1
	
	if(!result){
		printk("platform_driver initiated  = %d \n", result);
		//result = platform_device_register(&pdev);
		//strlcpy(pdev.bus_id, "aaa",BUS_ID_SIZE);
		
		result = device_register(&ldd_bus);
		printk("1. platform_device_result = %d \n", result);

		//strncpy(pdev.dev.bus_id, pdev.name, BUS_ID_SIZE);
		//result = device_register(&pdev.dev);	
		result = ldd_device_register(&pdev);

		printk("2. platform_device_result = %d \n", result);
		if(result)
			//platform_driver_unregister(&s3c2410keypad_driver);
			driver_unregister(&s3c2410keypad_driver);
	}
	#endif
	
	/*
	ldddev->dev.bus	= &ldd_bus_type;
	ldddev->dev.parent	= &ldd_bus;
	ldddev->dev.release = ldd_dev_release;
	strncpy(ldddev->dev.bus_id, ldddev->name, BUS_ID_SIZE);
	return device_register(&ldddev->dev);
	*/
	printk(KERN_INFO "%s successfully loaded\n", DRV_NAME);

	return result;
    
}

static void __exit rebis_keyint_exit(void)
{
    
	cdev_del(&key_cdev);
	unregister_chrdev_region(key_dev, 1);

	//device_register(&ldddev.dev);
	#if 1
	//platform_device_unregister(&pdev);
	device_unregister(&pdev.dev);
	device_unregister(&ldd_bus);
	#endif
	//platform_driver_unregister(&s3c2410keypad_driver);
	driver_unregister(&s3c2410keypad_driver);
	
	class_device_unregister(&key_cdevice);
	//class_destroy(key_class);
	class_unregister(&key_class);

	bus_unregister(&ldd_bus_type);

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
