#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/slab.h>
#include<linux/mempool.h>

#define MIN_ELEMENT 4
#define TEST_ELEMENT 6

typedef struct{ 
	int number;
	char string[128];
}TMem;

int elementcount = 0;

void *mempool_alloc_test(int gfp_mask, void *poop_data){
	TMem *data;
	printk("\n----> mempool_alloc_test\n");

	if(elementcount > 4){
		printk("elementcount = %d\n",elementcount);
		return NULL;
	}

	data = kmalloc(sizeof(TMem), gfp_mask);
	if(data != NULL) data->number = elementcount++;
	return data;
}

void mempool_free_test(void *element, void *pool_data){
	printk("\n----> call mempool_free_test\n");
	if(element != NULL) kfree(element);
}

int mempool_init(void){
	mempool_t *mp;
	TMem *element[TEST_ELEMENT];
	int lp;

	printk("\nModule MEMPOOL TEST\n");
	memset(element, 0, sizeof(element));
	printk("call mempool_create\n");

	mp = mempool_create(MIN_ELEMENT, mempool_alloc_test, mempool_free_test, NULL);

	printk("mempool allocate\n");
	for(lp=0; lp<TEST_ELEMENT; lp++){
		printk("<for> lp = %d\n",lp);
		element[lp] = mempool_alloc(mp,GFP_KERNEL);
		if(element[lp] == NULL) printk("allocate fail\n");
		else{
			sprintf(element[lp]->string, "alloc data %d\n",element[lp]->number);
			printk(element[lp]->string);
		}
	}

	printk("mempoo free\n");
	for(lp=0; lp<TEST_ELEMENT; lp++){
		if(element[lp] != NULL) mempool_free(element[lp],mp);
	}

	printk("call mempool_destroy\n");
	mempool_destroy(mp);
	return 0;
}

void mempool_exit(void){
	printk("\nModule MEMPOOL test end\n");
}

module_init(mempool_init);
module_exit(mempool_exit);

MODULE_LICENSE("Dual BSD/GPL");
