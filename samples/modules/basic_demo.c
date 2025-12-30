/*
 * basic_demo.c - 一个简单的Linux内核模块示例
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init basic_demo_init(void)
{
	pr_info("basic_demo: Module loaded\n");
	return 0;
}

static void __exit basic_demo_exit(void)
{
	pr_info("basic_demo: Module unloaded\n");
}

module_init(basic_demo_init);
module_exit(basic_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module example");
MODULE_VERSION("1.0");
