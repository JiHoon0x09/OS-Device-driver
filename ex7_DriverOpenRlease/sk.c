#include<linux/module.h>
#include<linux/init.h>
#include<linux/major.h>
#include<linux/fs.h>
#include<linux/cdev.h>

MODULE_LICENSE("GPL");

static int sk_major = 203, sk_minor = 0;
static int result;
static dev_t sk_dev;

static struct cdev sk_cdev;
struct file_operations sk_fops;
static int sk_register_cdev(void);

static int sk_open(struct inode *inode, struct file *filp);
static int sk_release(struct inode *inode, struct file *filp);

static int sk_open(struct inode *inode, struct file *filp)
{
	printk("Device has been opened....\n");
	/* HW initailization */
	/* MOD_IND_USE_COUNT; */
	return 0;
}

static int sk_release(struct inode *inode, struct file *filp)
{
	printk("Device has been Closed....\n");
	return 0;
}

struct file_operations sk_fops = {
	.open 	= sk_open,
	.release = sk_release,
};


static __init int sk_init(void)
{
	printk("SK Module is up...\n\n");

	if((result = sk_register_cdev())< 0){
		return result;
	}
	return 0;
}

static __exit void sk_exit(void)
{
	printk("The module is down...\n\n");
	cdev_del(&sk_cdev);
	unregister_chrdev_region(sk_dev, 1);
}

static int sk_register_cdev(void)
{
	int error;

	if(sk_major){
		sk_dev = MKDEV(sk_major, sk_minor);
		error = register_chrdev_region(sk_dev, 1, "sk");
	}
	else{
		error = alloc_chrdev_region(&sk_dev, sk_minor, 1, "sk");
		sk_major = MAJOR(sk_dev);
	}

	if(error<0){
		printk(KERN_WARNING "sk: Can't get major %d\n\n",sk_major);
		return error;
	}

	printk("Major Number = %d\n\n",sk_major);

	cdev_init(&sk_cdev, &sk_fops);
	sk_cdev.owner = THIS_MODULE;
	sk_cdev.ops = &sk_fops;
	error = cdev_add(&sk_cdev, sk_dev, 1);

	if(error)
		printk(KERN_NOTICE "sk Register Error %d\n\n",error);

	return 0;
}


module_init(sk_init);
module_exit(sk_exit);
