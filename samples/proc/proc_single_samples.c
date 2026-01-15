/*
 * proc_samples.c - Linux proc 文件系统示例模块
 *
 * 创建 /proc/samples_node 文件，演示函数接口调用.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

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

static int node_open(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t node_read(struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
	return 0;
}
static ssize_t node_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset)
{
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
