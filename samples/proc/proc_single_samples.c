/*
 * proc_samples.c - Linux proc 文件系统示例模块
 *
 * 创建 /proc/samples_node 文件，演示函数接口调用.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>

#include <linux/printk.h>


static struct proc_dir_entry *samples_node;

static int node_open(struct inode *inode, struct file *file);
static ssize_t node_read(struct file *file, char __user *buffer, size_t count, loff_t *offset);
static ssize_t node_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset);

static const struct file_operations node_ops = {
	.owner		= THIS_MODULE,
	.open		= node_open,
	.read		= node_read,
	.write		= node_write,
	.llseek		= NULL,
	.release	= NULL,
};

/*
 * 即使没有open 操作，读写也可以正常执行.
 * 所以这里的open 操作更像是一个hook点，允许用户在open 的时候执行具体的动作.
 */
/* 文件描述符是上层维护的，这里的返回值和文件描述符没有关系. */
static int node_open(struct inode *inode, struct file *file)
{
	dump_stack();

	/* 返回0 表示成功 */
	return 0;
}
static ssize_t node_read(struct file *file, char __user *buffer, size_t usize, loff_t *offset)
{
	int ret;
	int size = sizeof("aa\n");

	if (*offset != 0) {
		return 0;
	}

	ret = copy_to_user(buffer, "aa\n", size);

	/* 需要手动更新offset */
	*offset += (size - ret);

	return (size - ret);
}

/* offset 实际是本地使用的，用于控制从哪里读取，写到哪里去 */
static ssize_t node_write(struct file *file, const char __user *buffer, size_t usize, loff_t *offset)
{
	pr_info("%s %d: usize %ld offset %lld\n", __func__, __LINE__, usize, *offset);

#if 1
	if (usize > 0)
		return usize;
#else
	/* 返回值小于写入的字节数，用户态会重复执行写入，直到完全写入成功 */
	if (usize > 0)
		return usize-1;
#endif

	return 0;
}

static int __init proc_demo_init(void)
{
	samples_node = proc_create("samples_node", 0, NULL, &node_ops);
	if (samples_node == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static void __exit proc_demo_exit(void)
{
	remove_proc_entry("samples_node", NULL);
	return;
}

module_init(proc_demo_init);
module_exit(proc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple proc fs example");
MODULE_VERSION("1.0");
