/*
 * hello.c - 简单的Linux内核模块示例
 * 适用于Linux 2.6.12
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_init(void)
{
	printk(KERN_INFO "Hello: init...\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Hello: exit...\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sample Author");
MODULE_DESCRIPTION("simple hello module");
MODULE_VERSION("1.0");
