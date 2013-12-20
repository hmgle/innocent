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

#define IDIOM_LEN 12
#define IDIOM_HASH_BITS 12
#define IDIOM_TABLE_SIZE (1 << IDIOM_HASH_BITS)

static struct hlist_head idiom_table[IDIOM_TABLE_SIZE];

static char prefix[3];

struct idiom_index {
	struct hlist_node hlist;
	char idiom_index_name[3];
	struct list_head list;
};

struct idiom_entry {
	char idiom[IDIOM_LEN];
	struct list_head list;
};

static struct idiom_index *get_idiom_index(const char *name)
{
	struct hlist_head *head;
#if NEWKERN
#else
	struct hlist_node *node;
#endif
	struct idiom_index *e;
	u32 hash = jhash(name, 3, 0);

	head = &idiom_table[hash & (IDIOM_TABLE_SIZE - 1)];
#if NEWKERN
	hlist_for_each_entry(e, head, hlist)
#else
	hlist_for_each_entry(e, node, head, hlist)
#endif
		if (!strncmp(name, e->idiom_index_name, 3))
			return e;
	return NULL;
}

static struct idiom_index *add_idiom_index(const char *name)
{
	struct hlist_head *head;
#if NEWKERN
#else
	struct hlist_node *node;
#endif
	struct idiom_index *e;
	u32 hash = jhash(name, 3, 0);

	head = &idiom_table[hash & (IDIOM_TABLE_SIZE - 1)];
#if NEWKERN
	hlist_for_each_entry(e, head, hlist)
#else
	hlist_for_each_entry(e, node, head, hlist)
#endif
		if (!strncmp(name, e->idiom_index_name, 3))
			return ERR_PTR(-EEXIST); /* Already there */
	e = kmalloc(sizeof(struct idiom_index), GFP_KERNEL);
	if (!e)
		return ERR_PTR(-ENOMEM);
	memcpy(&e->idiom_index_name[0], name, 3);
	INIT_LIST_HEAD(&e->list);
	hlist_add_head(&e->hlist, head);
	return e;
}

static void idiom_index_add_entry(struct idiom_index *index,
				struct idiom_entry *entry)
{
	list_add(&entry->list, &index->list);
}

static void idiom_index_del_all_entry(struct idiom_index *index)
{
	struct idiom_entry *tmp;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &index->list) {
		tmp = list_entry(pos, struct idiom_entry, list);
		list_del(pos);
		kfree(tmp);
	}
}

static void idiom_del_all_index(void)
{
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct idiom_index *index;
	int i;

	for (i = 0; i < IDIOM_TABLE_SIZE; i++) {
		head = &idiom_table[i];
		hlist_for_each_safe(node, tmp, head) {
			index = hlist_entry(node, struct idiom_index, hlist);
			idiom_index_del_all_entry(index);
			hlist_del(node);
			kfree(index);
		}
	}
}

static int idiom_add_entry(const char name[12])
{
	struct idiom_index *index;
	struct idiom_entry *entry;

	index = get_idiom_index(name);
	if (!index) {
		index = add_idiom_index(name);
		if (IS_ERR(index))
			return -1;
	}
	/* TODO: check  duplicate */
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -1;
	memcpy(entry->idiom, name, IDIOM_LEN);
	idiom_index_add_entry(index, entry);
	return 0;
}

static void init_idiom_data(struct file *fp)
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
		ret = idiom_add_entry(buf);
		if (ret < 0) {
			printk("idiom_add_entry() failed!\n");
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
	struct idiom_index *index;
	struct idiom_entry *entry;
	char idiom[IDIOM_LEN + 1] = {0,};

	if (*f_pos != 0)
		return 0;
	if (prefix[0] == '\0')
		return 0;
	index = get_idiom_index(prefix);
	if (!index)
		return 0;
	list_for_each_entry(entry, &index->list, list) {
		memcpy(idiom, entry->idiom, IDIOM_LEN);
		idiom[IDIOM_LEN] = '\n';
		copy_to_user(buf + offset, idiom, IDIOM_LEN + 1);
		offset += IDIOM_LEN + 1;
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

static void release_idiom_data(void)
{
	idiom_del_all_index();
}

static int __init
innocent_init(void)
{
	struct file *fp;
	mm_segment_t fs;
	int ret;

	printk("innocent init\n");

	fp = filp_open("idiom.txt", O_RDONLY, 0644);
	if (IS_ERR(fp)) {
		printk("open file error\n");
		return -1;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);
	init_idiom_data(fp);
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
	release_idiom_data();
	printk("innocent exit\n");
}

module_init(innocent_init);
module_exit(innocent_exit);

MODULE_LICENSE("GPL");
