/*
 * waitqueue_sample.c - 等待队列示例模块
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/printk.h>

static DECLARE_WAIT_QUEUE_HEAD(wq);

static struct task_struct *kthread;

static struct proc_dir_entry *proc_entry;

static int event_flag = 0;

static int wait_event_thread(void *data)
{
	while (!kthread_should_stop()) {
		pr_info("wait write to proc\n");

		wait_event_interruptible(wq, event_flag != 0 || kthread_should_stop());

		pr_info("wake up success\n");
		if (kthread_should_stop())
			break;

		event_flag = 0;
	}

	return 0;
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	event_flag = 1;

	wake_up_interruptible(&wq);

	return count;
}

static const struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.write = proc_write,
};

static int __init waitqueue_demo_init(void)
{
	int ret = -ENOMEM;

	kthread = kthread_run(wait_event_thread, NULL, "wait_event_thread");
	if (IS_ERR(kthread)) {
		pr_err("failed create kernel thread\n");
		goto err_kthread;
	}

	proc_entry = proc_create("waitqueue_sample", 0200, NULL, &proc_fops);
	if (!proc_entry) {
		pr_err("failed create proc entry\n");
		goto err_proc;
	}

	return 0;

err_proc:
	kthread_stop(kthread);
err_kthread:
	return ret;
}

static void __exit waitqueue_demo_exit(void)
{
	if (proc_entry)
		proc_remove(proc_entry);

	if (kthread)
		kthread_stop(kthread);
}

module_init(waitqueue_demo_init);
module_exit(waitqueue_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple waitqueue example");
MODULE_VERSION("1.0");
