/***************************************
 * Filename: blocking.c
 * Title: Blocking example
 * Desc: Implementation of system call
 ***************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

static DECLARE_WAIT_QUEUE_HEAD(wq);
static int flag = 0;
static int result;
static char data[20] = {0,};

/* Define Prototype of functions */
static int sleepy_open(struct inode *inode, struct file *filp);
static int sleepy_release(struct inode *inode, struct file *filp);
static int sleepy_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int sleepy_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);

/* Implementation of functions */
static int sleepy_open(struct inode *inode, struct file *filp)
{
    printk("Device has been opened...\n");
    
    /* H/W Initalization */
   
    return 0;
}

static int sleepy_release(struct inode *inode, struct file *filp)
{
    printk("Device has been closed...\n");
  
    return 0;
}

static int sleepy_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "precess %i (%s) awakening the readers...\n", current->pid, current->comm);
	copy_from_user(data, buf, count);
	flag = 1;
	wake_up_interruptible(&wq);

    return count;
}

static int sleepy_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "precess %i (%s) going to sleep\n", current->pid, current->comm);
	wait_event_interruptible(wq, flag != 0);
	flag = 0;
	printk(KERN_INFO "awaken %i (%s)\n", current->pid, current->comm);

	copy_to_user(buf, data, count);

    return 0;
}

struct file_operations sleepy_fops = { 
    .open       = sleepy_open,
    .release    = sleepy_release,
    .write      = sleepy_write,
    .read       = sleepy_read,

};

static int __init sleepy_init(void)
{
    printk("Sleepy Module is up... \n");
	result = register_chrdev(0, "SLEEPY", &sleepy_fops);
    if (result < 0 ) {
        printk("Couldn't get a major number...\n");
    }
    printk("major number=%d\n", result);
    return 0;
}

static void __exit sleepy_exit(void)
{
    printk("The module is down...\n");
    unregister_chrdev(result, "SK");
}

module_init(sleepy_init); 
module_exit(sleepy_exit); 
