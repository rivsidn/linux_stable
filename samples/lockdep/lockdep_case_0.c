/*
 * lockdep.c - 一个简单的Linux内核模块，用于调试lockdep
 *
 * 用于触发调试信息:
 *
 * [ INFO: possible circular locking dependency detected ]
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

static DEFINE_SPINLOCK(lock_a);
static DEFINE_SPINLOCK(lock_b);
static DEFINE_SPINLOCK(lock_c);

static struct task_struct *task_0;
static struct task_struct *task_1;

static int threadfn(void *reverse)
{
	while(!kthread_should_stop()) {
		if (!reverse) {
			spin_lock(&lock_a);
			spin_lock(&lock_b);
			spin_lock(&lock_c);
			spin_unlock(&lock_c);
			spin_unlock(&lock_b);
			spin_unlock(&lock_a);

			printk("lock a -> lock b -> lock c\n");
		} else {
			spin_lock(&lock_a);
			spin_lock(&lock_c);
			spin_lock(&lock_b);
			spin_unlock(&lock_b);
			spin_unlock(&lock_c);
			spin_unlock(&lock_a);

			printk("lock a -> lock c-> lock b\n");
		}

		msleep(3000);
	}

	return 0;
}

static int __init basic_demo_init(void)
{
	int ret = -ENOMEM;

	task_0 = kthread_run(threadfn, (void *)0, "lockdep-0");
	if (IS_ERR(task_0)) {
		goto error_0;
	}

	task_1 = kthread_run(threadfn, (void *)1, "lockdep-1");
	if (IS_ERR(task_1)) {
		goto error_1;
	}

	return 0;

error_1:
	kthread_stop(task_0);
error_0:
	return ret;
}

static void __exit basic_demo_exit(void)
{
	if (task_0)
		kthread_stop(task_0);

	if (task_1)
		kthread_stop(task_1);
}

module_init(basic_demo_init);
module_exit(basic_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple kernel module example");
MODULE_VERSION("1.0");
