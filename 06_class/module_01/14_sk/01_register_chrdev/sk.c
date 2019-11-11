/***************************************
 * Filename: sk.c
 * Title: Skeleton Device
 * Desc: register and unregister chrdev
 ***************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

static int sk_major = 0, sk_minor = 0;
static int result;
static dev_t sk_dev;
/* TODO: file_operations¿ ¿¿¿¿ ¿¿¿¿. */
static struct file_operations sk_fops;
static struct cdev sk_cdev;

static int sk_register_cdev(void);

static int sk_init(void)
{
    printk("SK Module is up... \n");
    /* TODO: ¿¿ ¿¿¿¿¿ ¿¿ */
	if((result = sk_register_cdev()) < 0)
	{
		return result;
	}

    return 0;
}

static void sk_exit(void)
{
    printk("The module is down...\n");
    /* TODO: ¿¿ ¿¿¿¿¿ ¿¿ */
	cdev_del(&sk_cdev);
	unregister_chrdev_region(sk_dev, 1);
}

static int sk_register_cdev(void)
{
	int error;

	/* allocation device number */
	if(sk_major) {
		sk_dev = MKDEV(sk_major, sk_minor);
		error = register_chrdev_region(sk_dev, 1, "sk");
	} else {
		error = alloc_chrdev_region(&sk_dev, sk_minor, 1, "sk");
		sk_major = MAJOR(sk_dev);
	}

	if(error < 0) {
		printk(KERN_WARNING "sk: can't get major %d\n", sk_major);
		return result;
	}
    printk("major number=%d\n", sk_major);

	/* register chrdev */
	cdev_init(&sk_cdev, &sk_fops);
	sk_cdev.owner = THIS_MODULE;
	sk_cdev.ops = &sk_fops;
	error = cdev_add(&sk_cdev, sk_dev, 1);

	if(error)
		printk(KERN_NOTICE "sk Register Error %d\n", error);

	return 0;
}

module_init(sk_init); 
module_exit(sk_exit); 

