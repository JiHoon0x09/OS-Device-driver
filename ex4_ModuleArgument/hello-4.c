#include<linux/module.h>
#include<linux/moduleparam.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/stat.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hoon Ji Kim");
static int myint = 420;
static char *mystring = "blah";
static int myintArray[2] = {-1, 1};
static int arr_argc = 0;

module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(myint, "An Integer");
module_param(mystring, charp, 0000);
MODULE_PARM_DESC(mystrin, "A character string");
module_param_array(myintArray, int, &arr_argc, 0000);
MODULE_PARM_DESC(myintArray, "An array of Integers");

static int __init hello_4_init(void)
{
	int i;
	printk(KERN_INFO "Hello, World 4\n=====================\n");
	printk(KERN_INFO "myint is integer : %d\n",myint);
	printk(KERN_INFO "mystring is a string : %s\n",mystring);
	for(i=0; i<(sizeof myintArray/sizeof(int)); i++){
		printk(KERN_INFO "myintArray[%d] = %d\n",i,myintArray[i]);
	}
	printk(KERN_INFO "Got %d arguments for myintArray\n",arr_argc);
	return 0;
}

static void __exit hello_4_exit(void)
{
	printk(KERN_INFO "Goodbye, World 4\n\n");
}

module_init(hello_4_init);
module_exit(hello_4_exit);





