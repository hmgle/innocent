#include <linux/init.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/jhash.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/module.h>

#include "config.h"

#define IDIMO_HASH_BITS 12
#define IDIMO_TABLE_SIZE (1 << IDIMO_HASH_BITS)

static struct hlist_head idimo_table[IDIMO_TABLE_SIZE];

static char prefix[3];

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
#if NEWKERN
#else
	struct hlist_node *node;
#endif
	struct idimo_index *e;
	u32 hash = jhash(name, 3, 0);

	head = &idimo_table[hash & (IDIMO_TABLE_SIZE - 1)];
#if NEWKERN
	hlist_for_each_entry(e, head, hlist)
#else
	hlist_for_each_entry(e, node, head, hlist)
#endif
		if (!strncmp(name, e->idimo_index_name, 3))
			return e;
	return NULL;
}

static struct idimo_index *add_idimo_index(const char *name)
{
	struct hlist_head *head;
#if NEWKERN
#else
	struct hlist_node *node;
#endif
	struct idimo_index *e;
	u32 hash = jhash(name, 3, 0);

	head = &idimo_table[hash & (IDIMO_TABLE_SIZE - 1)];
#if NEWKERN
	hlist_for_each_entry(e, head, hlist)
#else
	hlist_for_each_entry(e, node, head, hlist)
#endif
		if (!strncmp(name, e->idimo_index_name, 3))
			return ERR_PTR(-EEXIST); /* Already there */
	e = kmalloc(sizeof(struct idimo_index), GFP_KERNEL);
	if (!e)
		return ERR_PTR(-ENOMEM);
	memcpy(&e->idimo_index_name[0], name, 3);
	INIT_LIST_HEAD(&e->list);
	hlist_add_head(&e->hlist, head);
	return e;
}

static void idimo_index_add_entry(struct idimo_index *index,
				struct idimo_entry *entry)
{
	list_add(&entry->list, &index->list);
}

static void idimo_index_del_all_entry(struct idimo_index *index)
{
	struct idimo_entry *tmp;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &index->list) {
		tmp = list_entry(pos, struct idimo_entry, list);
		list_del(pos);
		kfree(tmp);
	}
}

static void idimo_del_all_index(void)
{
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct idimo_index *index;
	int i;

	for (i = 0; i < IDIMO_TABLE_SIZE; i++) {
		head = &idimo_table[i];
		hlist_for_each_safe(node, tmp, head) {
			index = hlist_entry(node, struct idimo_index, hlist);
			idimo_index_del_all_entry(index);
			hlist_del(node);
			kfree(index);
		}
	}
}

static int idimo_add_entry(const char name[12])
{
	struct idimo_index *index;
	struct idimo_entry *entry;

	index = get_idimo_index(name);
	if (!index) {
		index = add_idimo_index(name);
		if (IS_ERR(index))
			return -1;
	}
	/* TODO: check  duplicate */
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -1;
	memcpy(entry->idimo, name + 3, 9);
	idimo_index_add_entry(index, entry);
	return 0;
}

static void init_idimo_data(struct file *fp)
{
	loff_t pos;
	loff_t file_offset = 0;
	ssize_t vfs_read_retval;
	char buf[16] = {0,};
	int ret;

	for (;;) {
		pos = file_offset;
		vfs_read_retval = vfs_read(fp, buf, 14, &pos);
		if (vfs_read_retval < 14) {
			printk("end\n");
			break;
		}
		file_offset += vfs_read_retval;
		ret = idimo_add_entry(buf);
		if (ret < 0) {
			printk("idimo_add_entry() failed!\n");
			return;
		}
	}
}

static long innocent_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	return 0;
}

static ssize_t innocent_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	char tmp[1020];

	printk("count is %d\n", count);
	if (count < 1020)
		copy_from_user(tmp, buf, count);
	else
		copy_from_user(tmp, buf, 1020);
	memcpy(prefix, tmp, 3);
	return count;
}

static ssize_t innocent_read(struct file *filp, char __user *buf,
			size_t count, loff_t *f_pos)
{
	int offset = 0;
	struct idimo_index *index;
	struct idimo_entry *entry;
	char idimo[16] = {0,};

	if (*f_pos != 0)
		return 0;
	if (prefix[0] == '\0')
		return 0;
	index = get_idimo_index(prefix);
	if (!index)
		return 0;
	memcpy(idimo, prefix, 3);
	list_for_each_entry(entry, &index->list, list) {
		memcpy(idimo + 3, entry->idimo, 9);
		idimo[12] = '\n';
		copy_to_user(buf + offset, idimo, 13);
		offset += 13;
	}
	*f_pos = offset;
	return offset;
}

static const struct file_operations innocent_fops = {
	.owner		= THIS_MODULE,
	.read		= innocent_read,
	.write		= innocent_write,
	.unlocked_ioctl	= innocent_ioctl,
};

static struct miscdevice innocent_dev = {
	MISC_DYNAMIC_MINOR,
	"innocent",
	&innocent_fops
};

static void release_idimo_data(void)
{
	idimo_del_all_index();
}

static int __init
innocent_init(void)
{
	struct file *fp;
	mm_segment_t fs;
	int ret;

	printk("innocent init\n");

	fp = filp_open("idimo.txt", O_RDONLY, 0644);
	if (IS_ERR(fp)) {
		printk("open file error\n");
		return -1;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);
	init_idimo_data(fp);
	filp_close(fp, NULL);
	set_fs(fs);

	ret = misc_register(&innocent_dev);
	if (ret)
		printk(KERN_ERR
			"Unable to register \"innocent\" misc device\n");
	return ret;
}

static void __exit
innocent_exit(void)
{
	misc_deregister(&innocent_dev);
	release_idimo_data();
	printk("innocent exit\n");
}

module_init(innocent_init);
module_exit(innocent_exit);

MODULE_LICENSE("GPL");
