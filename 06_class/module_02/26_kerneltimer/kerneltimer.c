/***************************************
 * Filename: kerneltimer.c
 * Title: Kernel Timer Handler
 * Desc: Timer Handler Module
 ***************************************/
//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>

#include <asm/arch/regs-gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/time.h>
#include <linux/timer.h>

#define   TIME_STEP           (2*HZ/10)

typedef struct
{
        struct timer_list  timer;            
	unsigned long      led;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;
static KERNEL_TIMER_MANAGER *ptrmng = NULL;

void kerneltimer_timeover(unsigned long arg );

void kerneltimer_registertimer( KERNEL_TIMER_MANAGER *pdata, unsigned long timeover )
{
     init_timer( &(pdata->timer) );
     pdata->timer.expires  = get_jiffies_64() + timeover;
     pdata->timer.data     = (unsigned long) pdata      ;
     pdata->timer.function = kerneltimer_timeover       ;
     add_timer( &(pdata->timer) );
}

void kerneltimer_timeover(unsigned long arg )
{
   KERNEL_TIMER_MANAGER *pdata = NULL;     
   
   if( arg )
   {
      pdata = ( KERNEL_TIMER_MANAGER * ) arg;

       s3c2410_gpio_setpin(S3C2410_GPE11, (pdata->led & 0xFF));
       s3c2410_gpio_setpin(S3C2410_GPE12, (pdata->led & 0xFF));
       s3c2410_gpio_setpin(S3C2410_GPG5, (pdata->led & 0xFF));
      
       pdata->led = ~(pdata->led);

      kerneltimer_registertimer( pdata , TIME_STEP );
 }
}

int kerneltimer_init(void)
{
    ptrmng = kmalloc( sizeof( KERNEL_TIMER_MANAGER ), GFP_KERNEL );
    if( ptrmng == NULL ) return -ENOMEM;
     
    memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER ) );
     
    ptrmng->led = 0;
    kerneltimer_registertimer( ptrmng, TIME_STEP );
    
    	// Turn off LEDs and IO configure
	s3c2410_gpio_setpin(S3C2410_GPE11, 1);
	s3c2410_gpio_setpin(S3C2410_GPE12, 1);
	s3c2410_gpio_setpin(S3C2410_GPG5, 1);

	s3c2410_gpio_cfgpin(S3C2410_GPE11, S3C2410_GPE11_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPE12, S3C2410_GPE12_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPG5, S3C2410_GPG5_OUTP);
     
    return 0;
}

void kerneltimer_exit(void)
{
    if( ptrmng != NULL ) 
    {
        del_timer( &(ptrmng->timer) );
 kfree( ptrmng );
    }    
    s3c2410_gpio_setpin(S3C2410_GPE11, 1);    
    s3c2410_gpio_setpin(S3C2410_GPE12, 1);
    s3c2410_gpio_setpin(S3C2410_GPG5, 1);
}

module_init(kerneltimer_init);
module_exit(kerneltimer_exit);

MODULE_LICENSE("Dual BSD/GPL");

