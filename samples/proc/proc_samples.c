/*
 * proc_samples.c - Linux proc 文件系统示例模块
 *
 * 创建如图所示的目录结构.
 *
 * /proc/samples/
 * │
 * ├── dir_0/
 * │   ├── node_0
 * │   ├── node_1
 * │   └── node_2
 * │
 * ├── dir_1/
 * │   ├── node_0
 * │   ├── node_1
 * │   └── node_2
 * │
 * └── dir_2/
 *     └── subdir_0
 *            └── node_0
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/proc_fs.h>

static struct proc_dir_entry *samples_dir;
static struct proc_dir_entry *dir_0;
static struct proc_dir_entry *dir_0_node_0;
static struct proc_dir_entry *dir_0_node_1;
static struct proc_dir_entry *dir_0_node_2;
static struct proc_dir_entry *dir_1;
static struct proc_dir_entry *dir_1_node_0;
static struct proc_dir_entry *dir_1_node_1;
static struct proc_dir_entry *dir_1_node_2;
static struct proc_dir_entry *dir_2;

static const struct file_operations node_ops = {
	.owner		= THIS_MODULE,
	.open		= NULL,
	.read		= NULL,
	.llseek		= NULL,
	.release	= NULL,
};

static int proc_build_dir_0(void)
{
	int ret = -ENOMEM;

	dir_0_node_0 = proc_create("node_0", 0, dir_0, &node_ops);
	if (!dir_0_node_0) {
		pr_err("dir_0 failed to create node_0\n");
		goto err_node_0;
	}
	dir_0_node_1 = proc_create("node_1", 0, dir_0, &node_ops);
	if (!dir_0_node_1) {
		pr_err("dir_0 failed to create node_1\n");
		goto err_node_1;
	}
	dir_0_node_2 = proc_create("node_2", 0, dir_0, &node_ops);
	if (!dir_0_node_2) {
		pr_err("dir_0 failed to create node_2\n");
		goto err_node_2;
	}

	return 0;

err_node_2:
	remove_proc_entry("node_1", dir_0);
err_node_1:
	remove_proc_entry("node_0", dir_0);
err_node_0:
	return ret;
}

static void proc_clean_dir_0(void)
{
	remove_proc_entry("node_2", dir_0);
	remove_proc_entry("node_1", dir_0);
	remove_proc_entry("node_0", dir_0);
}

static int proc_build_dir_1(void)
{
	int ret = -ENOMEM;

	dir_1_node_0 = proc_create("node_0", 0, dir_1, &node_ops);
	if (!dir_1_node_0) {
		pr_err("dir_1 failed to create node_0\n");
		goto err_node_0;
	}
	dir_1_node_1 = proc_create("node_1", 0, dir_1, &node_ops);
	if (!dir_1_node_1) {
		pr_err("dir_1 failed to create node_1\n");
		goto err_node_1;
	}
	dir_1_node_2 = proc_create("node_2", 0, dir_1, &node_ops);
	if (!dir_1_node_2) {
		pr_err("dir_1 failed to create node_2\n");
		goto err_node_2;
	}

	return 0;

err_node_2:
	remove_proc_entry("node_1", dir_1);
err_node_1:
	remove_proc_entry("node_0", dir_1);
err_node_0:
	return ret;
}

static void proc_clean_dir_1(void)
{
	remove_proc_entry("node_2", dir_1);
	remove_proc_entry("node_1", dir_1);
	remove_proc_entry("node_0", dir_1);
}

static int __init proc_demo_init(void)
{
	int ret = -ENOMEM;

	samples_dir = proc_mkdir("samples", NULL);
	if (!samples_dir) {
		pr_err("failed to create proc samples\n");
		goto err_samples;
	}

	dir_0 = proc_mkdir("dir_0", samples_dir);
	if (!dir_0) {
		pr_err("failed to create proc dir_0\n");
		goto err_dir_0;
	}
	ret = proc_build_dir_0();
	if (ret) {
		pr_err("failed to build proc dir_0\n");
		goto err_build_dir_0;
	}

	ret = -ENOMEM;
	dir_1 = proc_mkdir("dir_1", samples_dir);
	if (!dir_1) {
		pr_err("failed to create proc dir_1\n");
		goto err_dir_1;
	}
	ret = proc_build_dir_1();
	if (ret) {
		pr_err("failed to build proc dir_1\n");
		goto err_build_dir_1;
	}

	ret = -ENOMEM;
	dir_2 = proc_mkdir("dir_2", samples_dir);
	if (!dir_2) {
		pr_err("failed to create proc dir_2\n");
		goto err_dir_2;
	}

	return 0;

err_dir_2:
	proc_clean_dir_1();
err_build_dir_1:
	remove_proc_entry("dir_1", samples_dir);
err_dir_1:
	proc_clean_dir_0();
err_build_dir_0:
	remove_proc_entry("dir_0", samples_dir);
err_dir_0:
	remove_proc_entry("samples", NULL);
err_samples:
	return ret;
}

static void __exit proc_demo_exit(void)
{
	remove_proc_entry("dir_2", samples_dir);

	proc_clean_dir_1();
	remove_proc_entry("dir_1", samples_dir);

	proc_clean_dir_0();
	remove_proc_entry("dir_0", samples_dir);

	remove_proc_entry("samples", NULL);

	return;
}

module_init(proc_demo_init);
module_exit(proc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple proc fs example");
MODULE_VERSION("1.0");
