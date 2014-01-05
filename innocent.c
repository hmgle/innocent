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

#define IDIOM_WORD_COUNT 4
#define WORD_LEN 3
#define IDIOM_LEN (IDIOM_WORD_COUNT * WORD_LEN)
#define IDIOM_HASH_BITS 12
#define IDIOM_TABLE_SIZE (1 << IDIOM_HASH_BITS)

static struct hlist_head idiom_table[IDIOM_WORD_COUNT][IDIOM_TABLE_SIZE];

static char prefix[WORD_LEN];
static int position = 0;

struct idiom_index {
	struct hlist_node hlist;
	char idiom_index_name[WORD_LEN];
	struct list_head list;
};

struct idiom_entry {
	char idiom[IDIOM_LEN];
	struct list_head lists[IDIOM_WORD_COUNT];
	int refer_cnt;
};

static struct idiom_index *get_idiom_index(const char *word, int loc)
{
	struct hlist_head *head;
#if NEWKERN
#else
	struct hlist_node *node;
#endif
	struct idiom_index *e;
	u32 hash;

	if (loc < 0 || loc >= IDIOM_WORD_COUNT)
		return NULL;
	hash = jhash(word, WORD_LEN, 0);
	head = &idiom_table[loc][hash & (IDIOM_TABLE_SIZE - 1)];
#if NEWKERN
	hlist_for_each_entry(e, head, hlist)
#else
	hlist_for_each_entry(e, node, head, hlist)
#endif
		if (!strncmp(word, e->idiom_index_name, WORD_LEN))
			return e;
	return NULL;
}

static struct idiom_index *add_idiom_index(const char *word, int loc)
{
	struct hlist_head *head;
#if NEWKERN
#else
	struct hlist_node *node;
#endif
	struct idiom_index *e;
	u32 hash;

	if (loc < 0 || loc >= IDIOM_WORD_COUNT)
		return ERR_PTR(-1);
	hash = jhash(word, WORD_LEN, 0);
	head = &idiom_table[loc][hash & (IDIOM_TABLE_SIZE - 1)];
#if NEWKERN
	hlist_for_each_entry(e, head, hlist)
#else
	hlist_for_each_entry(e, node, head, hlist)
#endif
		if (!strncmp(word, e->idiom_index_name, WORD_LEN))
			return ERR_PTR(-EEXIST); /* Already there */
	e = kmalloc(sizeof(struct idiom_index), GFP_KERNEL);
	if (!e)
		return ERR_PTR(-ENOMEM);
	memcpy(&e->idiom_index_name[0], word, WORD_LEN);
	INIT_LIST_HEAD(&e->list);
	hlist_add_head(&e->hlist, head);
	return e;
}

static void idiom_index_add_entry(struct idiom_index *index,
				  struct idiom_entry *entry, int loc)
{
	list_add(&entry->lists[loc], &index->list);
	entry->refer_cnt++;
}

static void idiom_index_del_all_entry(struct idiom_index *index, int loc)
{
	struct idiom_entry *tmp;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &index->list) {
		tmp = list_entry(pos, struct idiom_entry, lists[loc]);
		list_del(pos);
		tmp->refer_cnt--;
		if (tmp->refer_cnt == 0)
			kfree(tmp);
	}
}

static void idiom_del_all_index(void)
{
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct idiom_index *index;
	int i;
	int j;

	for (j = 0; j < IDIOM_WORD_COUNT; j++)
		for (i = 0; i < IDIOM_TABLE_SIZE; i++) {
			head = &idiom_table[j][i];
			hlist_for_each_safe(node, tmp, head) {
				index =
				    hlist_entry(node, struct idiom_index,
						hlist);
				idiom_index_del_all_entry(index, j);
				hlist_del(node);
				kfree(index);
			}
		}
}

static int idiom_add_entry(const char name[IDIOM_LEN])
{
	struct idiom_index *index;
	struct idiom_entry *entry;
	int loc;

	/* TODO: check  duplicate */
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -1;
	entry->refer_cnt = 0;
	memcpy(entry->idiom, name, IDIOM_LEN);
	for (loc = 0; loc < IDIOM_WORD_COUNT; loc++) {
		index = get_idiom_index(name + loc * WORD_LEN, loc);
		if (!index) {
			index = add_idiom_index(name + loc * WORD_LEN, loc);
			if (IS_ERR(index))
				return -1;
		}
		idiom_index_add_entry(index, entry, loc);
	}
	return 0;
}

static void init_idiom_data(struct file *fp)
{
	loff_t pos;
	loff_t file_offset = 0;
	ssize_t vfs_read_retval;
	char buf[16] = { 0, };
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

static void pre_parse(const char *data, char *prefix, int *position,
		      size_t count)
{
	const char *c;
	size_t l;
	size_t remain;

	c = skip_spaces(data);
	if (*c == '1' || *c == '2' || *c == '3' || *c == '4') {
		*position = *c++ - '1';
		c = skip_spaces(c);
	}
	l = c - data;
	remain = count - l;
	if (remain > WORD_LEN)
		memcpy(prefix, c, WORD_LEN);
}

static ssize_t innocent_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *f_pos)
{
	char tmp[1020];

	if (count < 1020)
		copy_from_user(tmp, buf, count);
	else
		copy_from_user(tmp, buf, 1020);

	pre_parse(tmp, prefix, &position, count);
	return count;
}

static ssize_t innocent_read(struct file *filp, char __user *buf,
			     size_t count, loff_t *f_pos)
{
	int offset = 0;
	struct idiom_index *index;
	struct idiom_entry *entry;
	char idiom[IDIOM_LEN + 1] = { 0, };
	unsigned long ret;

	if (*f_pos != 0)
		return 0;
	if (prefix[0] == '\0')
		return 0;
	index = get_idiom_index(prefix, position);
	if (!index)
		return 0;
	list_for_each_entry(entry, &index->list, lists[position]) {
		memcpy(idiom, entry->idiom, IDIOM_LEN);
		idiom[IDIOM_LEN] = '\n';
		ret = copy_to_user(buf + offset, idiom, IDIOM_LEN + 1);
		if (ret) {
			offset += IDIOM_LEN + 1 - ret;
			break;
		}
		offset += IDIOM_LEN + 1;
	}
	*f_pos = offset;
	return offset;
}

static const struct file_operations innocent_fops = {
	.owner = THIS_MODULE,
	.read = innocent_read,
	.write = innocent_write,
	.unlocked_ioctl = innocent_ioctl,
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

static int __init innocent_init(void)
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

static void __exit innocent_exit(void)
{
	misc_deregister(&innocent_dev);
	release_idiom_data();
	printk("innocent exit\n");
}

module_init(innocent_init);
module_exit(innocent_exit);

MODULE_LICENSE("GPL");
