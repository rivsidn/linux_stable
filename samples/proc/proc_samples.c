/*
 * proc_samples.c - Linux proc 文件系统示例模块
 *
 * 创建如图所示的目录结构.
 *
 * /proc/samples/
 * │
 * ├── dir_0/
 * │   ├── node0
 * │   ├── node1
 * │   └── node2
 * │
 * ├── dir_1/
 * │   ├── node0
 * │   ├── node1
 * │   └── node2
 * │
 * └── dir_2/
 *     └── subdir_0
 *            └── node0
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/proc_fs.h>

static struct proc_dir_entry *samples_dir;
static struct proc_dir_entry *dir_0;
static struct proc_dir_entry *dir_1;
static struct proc_dir_entry *dir_2;

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
	dir_1 = proc_mkdir("dir_1", samples_dir);
	if (!dir_1) {
		pr_err("failed to create proc dir_1\n");
		goto err_dir_1;
	}
	dir_2 = proc_mkdir("dir_2", samples_dir);
	if (!dir_2) {
		pr_err("failed to create proc dir_2\n");
		goto err_dir_2;
	}

	return 0;

err_dir_2:
	remove_proc_entry("dir_1", samples_dir);
err_dir_1:
	remove_proc_entry("dir_0", samples_dir);
err_dir_0:
	remove_proc_entry("samples", NULL);
err_samples:
	return ret;
}

static void __exit proc_demo_exit(void)
{
	remove_proc_entry("dir_2", samples_dir);
	remove_proc_entry("dir_1", samples_dir);
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
