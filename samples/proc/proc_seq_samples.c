/*
 * proc_samples.c - Linux proc 文件系统示例模块
 *
 * 创建 /proc/samples_node 文件，演示seq_xx 函数接口.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static struct proc_dir_entry *samples_node;


static const struct file_operations node_ops = {
	.owner		= THIS_MODULE,
	.open		= NULL,
	.read		= NULL,
	.llseek		= NULL,
	.release	= NULL,
};

static int __init proc_demo_init(void)
{
	return 0;
}

static void __exit proc_demo_exit(void)
{

	return;
}

module_init(proc_demo_init);
module_exit(proc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple proc fs example");
MODULE_VERSION("1.0");
