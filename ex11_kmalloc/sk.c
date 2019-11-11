#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

void kmalloc_test(void){
	char *buf;
	printk("k_test: kmalloc test\n");
	buf = kmalloc(1204, GFP_KERNEL);
	if(buf != NULL){
		sprintf(buf,"k: test memory\n");
		printk(buf);
		kfree(buf);
	}
	buf = kmalloc(32*PAGE_SIZE, GFP_KERNEL);
	if(buf != NULL){
		printk("k: BIG memory OK \n\n");
		kfree(buf);
	}
}
void vmalloc_test(void){
	char *buf;
	printk("v_test: vmalloc test\n");
	buf = vmalloc(33*PAGE_SIZE);
	if(buf != NULL){
		sprintf(buf, "v: vmalloc test ok\n\n");
		printk(buf);
		vfree(buf);
	}
}
void get_free_pages_test(void){
	char *buf;
	int order;
	printk("page: get_free_pages test\n");
	order = get_order(8192*10);
	buf = __get_free_pages(GFP_KERNEL, order);
	if(buf != NULL){
		sprintf(buf, "page: __get_free_pages test ok [%d]\n\n",order);
		printk(buf);
		free_pages(buf,order);
	}
}

int memtest_init(void){
	char *data;
	printk("Module memoty test\n");
	kmalloc_test();
	vmalloc_test();
	get_free_pages_test();
	return 0;
}

void memtest_exit(void){
	printk("Module Memory test end\n");
}

module_init(memtest_init);
module_exit(memtest_exit);

MODULE_LICENSE("Dual BSD/GPL");

