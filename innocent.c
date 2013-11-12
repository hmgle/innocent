#include <linux/init.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/jhash.h>
#include <linux/slab.h>
#include <linux/module.h>

#define IDIMO_HASH_BITS 12
#define IDIMO_TABLE_SIZE (1 << IDIMO_HASH_BITS)

static struct hlist_head idimo_table[IDIMO_TABLE_SIZE];

struct idimo_index {
	struct hlist_node hlist;
	char idimo_index_name[3];
	struct list_head list;
};

struct idimo_entry {
	char idimo[9];
	struct list_head list;
};

static struct idimo_index *get_idimo_index(const char *name) 
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct idimo_index *e;
	u32 hash = jhash(name, strlen(name), 0);

	head = &idimo_table[hash & (IDIMO_TABLE_SIZE - 1)];
	hlist_for_each_entry(e, node, head, hlist) {
		if (!strcmp(name, e->idimo_index_name))
			return e;
	}
	return NULL;
}

static struct idimo_index *add_idimo_index(const char *name)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct idimo_index *e;
	u32 hash = jhash(name, strlen(name), 0);

	head = &idimo_table[hash & (IDIMO_TABLE_SIZE - 1)];
	hlist_for_each_entry(e, node, head, hlist) {
		if (!strcmp(name, e->idimo_index_name))
			return ERR_PTR(-EEXIST); /* Already there */
	}
	e = kmalloc(sizeof(struct idimo_index), GFP_KERNEL);
	if (!e)
		return ERR_PTR(-ENOMEM);
	memcpy(&e->idimo_index_name[0], name, 3);
	hlist_add_head(&e->hlist, head);
	return e;
}

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
