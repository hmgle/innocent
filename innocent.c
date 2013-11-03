#include <linux/init.h>
#include <linux/module.h>

static void init_idimo_data(void)
{
}

static void release_idimo_data(void)
{
}

static int __init
innocent_init(void)
{
	printk("innocent init\n");
	init_idimo_data();
	return 0;
}

static void __exit
innocent_exit(void)
{
	release_idimo_data();
	printk("innocent exit\n");
}

module_init(innocent_init);
module_exit(innocent_exit);

MODULE_LICENSE("GPL");
