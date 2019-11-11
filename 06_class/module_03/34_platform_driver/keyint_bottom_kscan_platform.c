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

#include <linux/input.h> //input_dev

#define DRV_NAME		"keyint"
#define S3C2410BUTVERSION	0x001


#define	KEY_MATRIX_BASE1	0
#define	KEY_MATRIX_BASE2	1
#define	KEY_MATRIX_BASE3	2
#define	KEY_MATRIX_BASE4	3
#define	KEY_MATRIX_BASE5	4

#define EXAMPLE 200 
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

//static int key_major = 0, key_minor = 0;
//static int result;
//static dev_t key_dev;
//static struct cdev key_cdev;

//static char dev_name[] = "rebis_keyscan";
static int cur_key, old_key;
//static flag = 0;

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
//static int key_register_cdev(void);

static irqreturn_t
rebis_keyevent(int irq, void *dev_id, struct pt_regs *regs)
{
    struct rebis_key_detection *gd = (struct rebis_key_detection *) dev_id;
    int             state;

	state = 1;
	printk("gd= %x, keypad was pressed \n",(unsigned int)gd);

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

	//flag = 1;
	//wake_up_interruptible(&wq);

	return IRQ_HANDLED;

}

static int scan_input(void) {

	if(((readl(S3C2410_GPFDAT) >> 3) & 0x1) != 0x1)
	{
		if(!s3c2410_gpio_getpin(S3C2410_GPF3))
			return 1;
	}
	return 0;
}

#if (EXAMPLE == 100)
static void rebis_keyint_callback(void *pgd)
{
    //struct rebis_key_detection *gd = (struct rebis_key_detection *)pgd    
    //int state = gd->last_state;
	int i;

	gprintk("workqueue callback call\n\n");

	//for key scan
	#if 1
	cur_key = 0;
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_INP);
	for(i=4; i>=0; i--)
	{
		writel(readl(S3C2410_GPBDAT) | (0x1f), S3C2410_GPBDAT);
		writel(readl(S3C2410_GPBDAT) & (~(0x1 << i)), S3C2410_GPBDAT);
		
		cur_key = scan_input();
		if(cur_key)
		//cur_key = scan_input();
		{
			cur_key += (4-i);//key_base[i];
			if(cur_key == old_key)
				goto SameValue;
			old_key = cur_key;
			printk("cur_key = %d \n\n", cur_key);
			
			//put_user(cur_key,(char *)buff);
			break;
		}
	}
SameValue:
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
#elif (EXAMPLE == 200)
static void rebis_keyint_callback(ulong data)
{
    //struct rebis_key_detection *gd = (struct rebis_key_detection *)data;    
    //int state = gd->last_state;

	//int key_base[5] = {KEY_MATRIX_BASE5, KEY_MATRIX_BASE4, KEY_MATRIX_BASE3, KEY_MATRIX_BASE2, KEY_MATRIX_BASE1};
	int i;

	gprintk("tasklet callback call\n");

	//for key scan
	#if 1
	cur_key = 0;
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_INP);
	for(i=4; i>=0; i--)
	{
		writel(readl(S3C2410_GPBDAT) | (0x1f), S3C2410_GPBDAT);
		writel(readl(S3C2410_GPBDAT) & (~(0x1 << i)), S3C2410_GPBDAT);
		
		cur_key = scan_input();
		if(cur_key)
		//cur_key = scan_input();
		{
			cur_key += (4-i);//key_base[i];
			if(cur_key == old_key)
				goto SameValue;
			old_key = cur_key;
			printk("cur_key = %d \n\n", cur_key);
			
			//put_user(cur_key,(char *)buff);
			break;
		}
	}
SameValue:
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
	//int i;
	int ret;
	//int error;
	//struct input_dev 	*input_dev;

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

static void release_pdev(struct device * dev){
	dev->parent 	= NULL;
}

static struct platform_device pdev  =
{
	.name	= "s3c2410-keypad",
	.id		= -1,
	.dev	= {
		.release	= release_pdev,
	},
};

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
	int result;
	result = platform_driver_register(&s3c2410keypad_driver);
	
	if(!result){
		printk("platform_driver initiated  = %d \n", result);
		result = platform_device_register(&pdev);
		printk("platform_device_result = %d \n", result);
		if(result)
			platform_driver_unregister(&s3c2410keypad_driver);
	}
	printk(KERN_INFO "%s successfully loaded\n", DRV_NAME);

	return result;
    
}

static void __exit rebis_keyint_exit(void)
{
	platform_device_unregister(&pdev);
	platform_driver_unregister(&s3c2410keypad_driver);

    printk(KERN_INFO "%s successfully removed\n", DRV_NAME);
}

module_init(rebis_keyint_init);
module_exit(rebis_keyint_exit);

MODULE_AUTHOR("Jurngyu,Park <jurngyu@mdstec.com>");
MODULE_LICENSE("GPL");

/********************************************************
* platform device 등록 과정 
M2 board : arch/arm/mach-s3c2440/mach-mep2440.c

static struct platform_device *mep2440_devices[] __initdata = {
    &s3c_device_usb,
    &s3c_device_lcd,
    &s3c_device_wdt,
    &s3c_device_i2c,
    &s3c_device_iis,
    &mep2440_dm9k0,
    &s3c_device_nand,
	&s3c_device_keypad, --> 추가

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
