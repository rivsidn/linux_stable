/*
 * lockdep.c - 一个简单的Linux内核模块，用于调试lockdep
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/spinlock.h>

static int trace = 1;

static DEFINE_SPINLOCK(lock);

/* case 0: 进程内核态获取锁，不释放退出. */

static int  trace_init_0(void)
{
	spin_lock(&lock);
	return 0;
}
static void trace_exit_0(void)
{
	spin_unlock(&lock);
}

/* case 1: 进程重复获取锁 */

static int  trace_init_1(void)
{
	spin_lock(&lock);
	spin_lock(&lock);
	return 0;
}
static void trace_exit_1(void)
{
	;
}

static struct trace_func {
	int (*init)(void);
	void (*exit)(void);
} trace_funcs[] = {
	{
		trace_init_0,
		trace_exit_0
	},
	{
		trace_init_1,
		trace_exit_1
	}
};

static int __init basic_demo_init(void)
{
	int ret;

	if (trace >= ARRAY_SIZE(trace_funcs)) {
		return 0;
	}

	ret = trace_funcs[trace].init();
	if (ret) {
		pr_err("%d init error\n", trace);
		return -1;
	}

	return 0;
}

static void __exit basic_demo_exit(void)
{
	if (trace >= ARRAY_SIZE(trace_funcs)) {
		return;
	}

	trace_funcs[trace].exit();
}

module_init(basic_demo_init);
module_exit(basic_demo_exit);

module_param(trace, int, 0644);
MODULE_PARM_DESC(trace, "Trace case");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple kernel module example");
MODULE_VERSION("1.0");
