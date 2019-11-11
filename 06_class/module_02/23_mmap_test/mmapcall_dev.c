/****************************************
 *	Filename: mmapcall_dev.c
 *	Title: MMAP Test Driver
 *	Desc:
 ****************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <asm/page.h>
#include <linux/mm.h>

#define	MMAPCALL_DEV_NAME		"mmapcall"
#define	MMAPCALL_DEV_MAJOR		240

MODULE_LICENSE("Dual BSD/GPL");

static int mmapcall_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int mmapcall_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int mmapcall_mmap(struct file *filp, struct vm_area_struct *vma)
{
	printk("vm_pgoff	[%08X]\n", vma->vm_pgoff << PAGE_SHIFT);

	/* process에 할당될 메모리 주소 */
	printk("vm_start	[%08X]\n", vma->vm_start);
	/* process에 할당될 메모리 주소 끝 */
	printk("vm_end		[%08X]\n", vma->vm_end);
	/* 할당 요구한 크기 */
	printk("length		[%08X]\n", (vma->vm_end - vma->vm_start));
	/* 상태 플래그 */
	printk("vm_flags	[%08X]\n", vma->vm_flags);

	/* mmap이 실제로 구현되고 있지 않으므로 상태만 찍고 EAGAIN으로 반환 */
	return -EAGAIN;
}

struct file_operations mmapcall_fops =
{
	.owner		= THIS_MODULE,
	.open		= mmapcall_open,
	.release	= mmapcall_release,
	.mmap		= mmapcall_mmap,
};

static int mmapcall_init(void)
{
	int result;

	result = register_chrdev(MMAPCALL_DEV_MAJOR, MMAPCALL_DEV_NAME, &mmapcall_fops);
	if(result < 0)	return result;
	printk(KERN_INFO "Register Character Device Major Number: %d\n", result);

	return 0;
}

static void mmapcall_exit(void)
{
	unregister_chrdev(MMAPCALL_DEV_MAJOR, MMAPCALL_DEV_NAME);
}

module_init(mmapcall_init);
module_exit(mmapcall_exit);
