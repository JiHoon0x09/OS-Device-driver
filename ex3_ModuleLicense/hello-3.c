#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>

#define DRIVER_AUTHOR "HOON JI KIM"
#define DRIVER_DESC "A Sample drive"

static int __init init_hello_3(void)
{
	printk(KERN_INFO "Hello, World 3!!\n\n");
	return 0;
}

static int __exit cleanup_hello_3(void)
{
	printk(KERN_INFO "Goodbye, World 3!!\n\n");
}
module_init(init_hello_3);
module_exit(cleanup_hello_3);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("Test Device");
