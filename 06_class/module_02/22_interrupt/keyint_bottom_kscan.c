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


#include <linux/workqueue.h>


#define DRV_NAME		"keyint"

#define	KEY_MATRIX_BASE1	0
#define	KEY_MATRIX_BASE2	2
#define	KEY_MATRIX_BASE3	4
#define	KEY_MATRIX_BASE4	6
#define	KEY_MATRIX_BASE5	8

#define EXAMPLE 100 
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

static int cur_key, old_key;

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

static irqreturn_t
rebis_keyevent(int irq, void *dev_id, struct pt_regs *regs)
{
    struct rebis_key_detection *gd = (struct rebis_key_detection *) dev_id;
    int             state;

	state = 1;
	printk("<<< gd= %x, keypad was pressed >>>\n",(unsigned int)gd);

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
    //struct rebis_key_detection *gd = (struct rebis_key_detection *)pgd;
    //int state = gd->last_state;
	int key_base[5] = {KEY_MATRIX_BASE1, KEY_MATRIX_BASE2, KEY_MATRIX_BASE3, KEY_MATRIX_BASE4, KEY_MATRIX_BASE5};
	int i;

	gprintk("workqueue callback call\n");

	//key scan
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
			cur_key += key_base[i];
			if(cur_key == old_key)
				goto Same_Value;
			old_key = cur_key;
			gprintk("cur_key = %d \n\n", cur_key);
			//put_user(cur_key,(char *)buff);
			break;
		}
	}
Same_Value:

	old_key = 0;

	// set GPBDAT 0
	s3c2410_gpio_setpin(S3C2410_GPB0, 0);
	s3c2410_gpio_setpin(S3C2410_GPB1, 0);
	s3c2410_gpio_setpin(S3C2410_GPB2, 0);
	s3c2410_gpio_setpin(S3C2410_GPH9, 0);
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

	int key_base[5] = {KEY_MATRIX_BASE1, KEY_MATRIX_BASE2, KEY_MATRIX_BASE3, KEY_MATRIX_BASE4, KEY_MATRIX_BASE5};
	int i;

	gprintk("tasklet callback call\n");

	//key scan
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
			cur_key += key_base[i];
			if(cur_key == old_key)
				goto SameValue;
			old_key = cur_key;
			gprintk("cur_key = %d \n\n", cur_key);
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
	s3c2410_gpio_setpin(S3C2410_GPH9, 0);
	s3c2410_gpio_setpin(S3C2410_GPB4, 0);
	
	// change External Interrupts
	s3c2410_gpio_cfgpin(S3C2410_GPF3, S3C2410_GPF3_EINT3);
	#endif

}
#endif

static int __init rebis_keyint_init(void)
{
	int ret;

		// set output mode
	s3c2410_gpio_cfgpin(S3C2410_GPB0, S3C2410_GPB0_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB2, S3C2410_GPB2_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPH9, S3C2410_GPH9_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB4, S3C2410_GPB4_OUTP);
	// set data
	s3c2410_gpio_setpin(S3C2410_GPB0, 0);
	s3c2410_gpio_setpin(S3C2410_GPB1, 0);
	s3c2410_gpio_setpin(S3C2410_GPB2, 0);
	s3c2410_gpio_setpin(S3C2410_GPH9, 0);
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
	//INIT_WORK(&rebis_gd.gdetect, rebis_keyint_callback, &rebis_gd); //2.6.17 available
	INIT_WORK(&rebis_gd.gdetect, rebis_keyint_callback);
	#elif (EXAMPLE == 200)
	tasklet_init(&rebis_gd.gdetect, rebis_keyint_callback, (unsigned long)(&rebis_gd)); 
	#endif

	printk(KERN_INFO "%s successfully loaded\n", DRV_NAME);

    return 0;
    
}

static void __exit rebis_keyint_exit(void)
{
    free_irq(rebis_gd.irq, &rebis_gd);

	#if (EXAMPLE == 100)
	#elif (EXAMPLE == 200)
	tasklet_kill(&rebis_gd.gdetect);
	#endif

    printk(KERN_INFO "%s successfully removed\n", DRV_NAME);
}


module_init(rebis_keyint_init);
module_exit(rebis_keyint_exit);

MODULE_LICENSE("GPL");

