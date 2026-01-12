/*
 * waitqueue_exclusive_sample.c - 互斥等待队列示例模块
 *
 * 互斥队列一次只能唤醒一个，唤醒之后如果不满足条件，则将唤醒的进程
 * 放到将等待队列末尾，等待下次唤醒.
 *
 * 非互斥队列每次都会被遍历所有.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/printk.h>

static DECLARE_WAIT_QUEUE_HEAD(wq);

#define KTHREAD_MAX	8

struct kthread_data {
	int nr;
	int flag;
	int exclusive;
};

static struct kthread_data data[KTHREAD_MAX];

static struct task_struct *kthread[KTHREAD_MAX];

static struct proc_dir_entry *proc_entry;

static int wait_event_thread(void *data)
{
	struct kthread_data *local = (struct kthread_data *)data;

	while (!kthread_should_stop()) {
		pr_info("wait write to proc thread-%d%s\n", local->nr, (local->nr%2) ? "-ex": "");

		if (local->exclusive) {
			wait_event_interruptible_exclusive(wq, local->flag != 0 || kthread_should_stop());
		} else {
			wait_event_interruptible(wq, local->flag != 0 || kthread_should_stop());
		}

		pr_info("wake up thread-%d%s\n", local->nr, (local->nr%2) ? "-ex": "");
		if (kthread_should_stop())
			break;

		local->flag = 0;
	}

	return 0;
}

/* 可以看到头插、尾插的区别 */
static int proc_show(struct seq_file *m, void *v)
{
	wait_queue_t *curr;
	struct task_struct *task;
	unsigned long flags;
	int count = 0;

	seq_printf(m, "Wait Queue Status:\n");
	seq_printf(m, "%-6s %-20s %-8s %s\n", "PID", "COMM", "STATE", "FLAGS");
	seq_printf(m, "----------------------------------------------\n");

	/* 获取等待队列自旋锁 */
	spin_lock_irqsave(&wq.lock, flags);

	/* 遍历等待队列 */
	list_for_each_entry(curr, &wq.task_list, task_list) {
		/* 获取 task_struct */
		task = (struct task_struct *)curr->private;
		if (!task)
			continue;

		seq_printf(m, "%-6d %-20s %-8ld %s\n",
			   task->pid,
			   task->comm,
			   task->state,
			   (curr->flags & WQ_FLAG_EXCLUSIVE) ? "EXCLUSIVE" : "NORMAL");
		count++;
	}

	spin_unlock_irqrestore(&wq.lock, flags);

	seq_printf(m, "----------------------------------------------\n");
	seq_printf(m, "Total waiting tasks: %d\n", count);

	return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, NULL);
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	int i;
	int ret;
	char kbuf[16];
	size_t len;

	if (count == 0)
		return -EINVAL;

	len = min(count, sizeof(kbuf) - 1);

	if (copy_from_user(kbuf, buffer, len))
		return -EFAULT;

	kbuf[len] = '\0';

	ret = kstrtoint(kbuf, 10, &i);
	if (ret)
		return ret;

	if (i < 0 || i >= KTHREAD_MAX)
		return -EINVAL;

	data[i].flag = 1;

	wake_up_interruptible(&wq);

	return count;
}

static const struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
	.write = proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init waitqueue_demo_init(void)
{
	int i, ret;

	/* 创建多个内核线程 */
	for (i = 0; i < KTHREAD_MAX; i++) {
		data[i].nr = i;
		data[i].flag = 0;
		if (i%2) {
			data[i].exclusive = 1;
			kthread[i] = kthread_run(wait_event_thread, (void *)&data[i], "wait_thread-%d-ex", i);
		} else  {
			data[i].exclusive = 0;
			kthread[i] = kthread_run(wait_event_thread, (void *)&data[i], "wait_thread-%d", i);
		}

		if (IS_ERR(kthread[i])) {
			ret = PTR_ERR(kthread[i]);
			pr_err("failed create kernel thread\n");
			goto err;
		}
	}

	proc_entry = proc_create("waitqueue_exclusive_sample", 0600, NULL, &proc_fops);
	if (!proc_entry) {
		ret = -ENOMEM;
		pr_err("failed create proc entry\n");
		goto err;
	}

	return 0;

err:
	while (i--) {
		kthread_stop(kthread[i]);
	}

	return ret;
}

static void __exit waitqueue_demo_exit(void)
{
	int i;

	if (proc_entry)
		proc_remove(proc_entry);

	/*
	 * 为什么模块卸载的时候会自动唤醒等待队列进程？
	 *
	 * kthread_stop()会调用到wake_up_process() 函数.
	 */
	for (i = 0; i < KTHREAD_MAX; i++) {
		if (kthread[i])
			kthread_stop(kthread[i]);
	}
}

module_init(waitqueue_demo_init);
module_exit(waitqueue_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple waitqueue example");
MODULE_VERSION("1.0");
