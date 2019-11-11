#include<linux/errno.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/slab.h>
#include<linux/init.h>
#include<linux/delay.h>
#include<linux/interrupt.h>
#include<linux/device.h>
#include<asm/io.h>
#include<asm/irq.h>
#include<mach/gpio.h>
#include<mach/regs-gpio.h>
#include<plat/gpio-cfg.h>

#define DRV_NAME "keyint"

static irqreturn_t keyinterrupt_func(int irq, void *dev_id, struct pt_regs *regs){
	printk("\n keypad was pressed\n");
	return IRQ_HANDLED;
}

static int __init rebis_keyint_init(void){
	int ret;
	s3c_gpio_cfgpin(S3C2410_GPF(0), S3C_GPIO_SFN(2));

	if(request_irq(IRQ_EINT0, (void*)keyinterrupt_func,IRQF_DISABLED|IRQF_TRIGGER_FALLING,DRV_NAME, NULL)){
		printk("Failed to request external interrupt \n");
		ret = -ENOENT;
		return ret;
	}

	printk(KERN_INFO "%s successfully loaded\n",DRV_NAME);

	return 0;
}

static void __exit rebis_keyint_exit(void){
	free_irq(IRQ_EINT0, NULL);
	printk(KERN_INFO "%s successfully removed \n",DRV_NAME);
}

module_init(rebis_keyint_init);
module_exit(rebis_keyint_exit);

MODULE_LICENSE("GPL");

