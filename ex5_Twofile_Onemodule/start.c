#include<linux/kernel.h>
#include<linux/module.h>

int init_module(void)
{
	printk(KERN_INFO "Hello, World - this is the kernel speaking\n\n");
	return 0;
}

