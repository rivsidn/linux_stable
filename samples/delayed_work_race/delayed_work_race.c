/*
 * delayed_work_race.c - 演示delayed_work竞态条件
 *
 * 这个示例模块演示了类似CIFS中的竞态条件：
 * 即使调用了cancel_delayed_work_sync，工作函数仍然可能重新排队自己
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kthread.h>

struct server_info {
	struct delayed_work echo_work;
	int status;
	int echo_count;
	bool should_exit;
	spinlock_t lock;
};

#define STATUS_NORMAL    0
#define STATUS_EXITING   1

static struct task_struct *test_thread;

/* 模块参数 */
static int test_case = 4;
module_param(test_case, int, 0644);
MODULE_PARM_DESC(test_case, "Test case to run: 0=all, 1=wrong_order, 2=correct_order, 3=with_patch, 4=race_demo (default: 0)");

/* 模拟cifs_echo_request */
static void echo_request(struct work_struct *work)
{
	struct server_info *srv = container_of(work, struct server_info, echo_work.work);
	unsigned long flags;
	int status;

	spin_lock_irqsave(&srv->lock, flags);
	status = srv->status;
	srv->echo_count++;
	printk(KERN_INFO "[%d] echo_request: status=%d, count=%d\n",
		task_pid_nr(current), status, srv->echo_count);
	spin_unlock_irqrestore(&srv->lock, flags);

	/* 模拟一些处理时间 */
	msleep(10);

	/* 检查状态 - 类似cifs_echo_request第444行 */
	spin_lock_irqsave(&srv->lock, flags);
	if (srv->status == STATUS_EXITING) {
		printk(KERN_INFO "[%d] echo_request: detected EXITING, but still requeue!\n",
			task_pid_nr(current));
	}
	spin_unlock_irqrestore(&srv->lock, flags);

	/* 总是重新排队自己 - 类似cifs_echo_request第456行 */
	printk(KERN_INFO "[%d] echo_request: requeuing work\n", task_pid_nr(current));
	queue_delayed_work(system_wq, &srv->echo_work, HZ); /* 1秒后再次执行 */
}

/* 模拟cifs_put_tcp_session */
static void put_session_wrong_order(struct server_info *srv)
{
	printk(KERN_INFO "\n=== Wrong Order (像原始代码) ===\n");

	/* 先取消工作 - 类似第2163行 */
	printk(KERN_INFO "[%d] Calling cancel_delayed_work_sync...\n", task_pid_nr(current));
	cancel_delayed_work_sync(&srv->echo_work);
	printk(KERN_INFO "[%d] cancel_delayed_work_sync returned\n", task_pid_nr(current));

	/* 模拟一些处理 */
	msleep(50);

	/* 后设置状态 - 类似第2179行 */
	spin_lock(&srv->lock);
	printk(KERN_INFO "[%d] Setting status to EXITING\n", task_pid_nr(current));
	srv->status = STATUS_EXITING;
	spin_unlock(&srv->lock);

	/* 等待看看是否有新的echo被排队 */
	msleep(1500);

	/* 检查echo_count是否还在增加 */
	printk(KERN_INFO "[%d] Final echo_count: %d\n", task_pid_nr(current), srv->echo_count);
	if (srv->echo_count > 1) {
		printk(KERN_ERR "!!! RACE DETECTED: echo still running after cancel !!!\n");
	}
}

/* 模拟修复后的顺序 */
static void put_session_correct_order(struct server_info *srv)
{
	printk(KERN_INFO "\n=== Correct Order (修复建议) ===\n");

	/* 先设置状态 */
	spin_lock(&srv->lock);
	printk(KERN_INFO "[%d] Setting status to EXITING first\n", task_pid_nr(current));
	srv->status = STATUS_EXITING;
	spin_unlock(&srv->lock);

	/* 后取消工作 */
	printk(KERN_INFO "[%d] Calling cancel_delayed_work_sync...\n", task_pid_nr(current));
	cancel_delayed_work_sync(&srv->echo_work);
	printk(KERN_INFO "[%d] cancel_delayed_work_sync returned\n", task_pid_nr(current));

	/* 等待看看是否有新的echo被排队 */
	msleep(1500);

	/* 检查echo_count是否还在增加 */
	printk(KERN_INFO "[%d] Final echo_count: %d\n", task_pid_nr(current), srv->echo_count);
}

/* 模拟clean_demultiplex_info with patch */
static void clean_with_patch(struct server_info *srv)
{
	printk(KERN_INFO "\n=== With Patch (在clean_demultiplex_info中再次取消) ===\n");

	/* 设置状态 - 类似clean_demultiplex_info第731行 */
	spin_lock(&srv->lock);
	srv->status = STATUS_EXITING;
	spin_unlock(&srv->lock);

	/* 补丁添加的取消操作 */
	printk(KERN_INFO "[%d] clean: calling cancel_delayed_work_sync\n", task_pid_nr(current));
	cancel_delayed_work_sync(&srv->echo_work);
	printk(KERN_INFO "[%d] clean: cancel done\n", task_pid_nr(current));

	/* 现在可以安全释放 */
	printk(KERN_INFO "[%d] clean: safe to free memory now\n", task_pid_nr(current));
}

/* 演示竞态条件的特殊测试 */
static void race_demo_test(void)
{
	struct server_info *srv;
	int i;

	printk(KERN_INFO "\n=== Race Condition Demo (重现竞态) ===\n");
	printk(KERN_INFO "这个测试会多次尝试触发竞态条件\n");

	for (i = 0; i < 100; i++) {
		srv = kzalloc(sizeof(*srv), GFP_KERNEL);
		if (!srv)
			return;

		spin_lock_init(&srv->lock);
		INIT_DELAYED_WORK(&srv->echo_work, echo_request);
		srv->status = STATUS_NORMAL;
		srv->echo_count = 0;

		printk(KERN_INFO "\n--- 尝试 %d ---\n", i + 1);

		/* 启动echo工作，使用很短的延迟增加竞态概率 */
		queue_delayed_work(system_wq, &srv->echo_work, 1);
		msleep(20); /* 很短的等待 */

		/* 尝试触发竞态 */
		printk(KERN_INFO "[%d] cancel_delayed_work_sync...\n", task_pid_nr(current));
		cancel_delayed_work_sync(&srv->echo_work);

		/* 极短延迟后设置状态 */
		udelay(100); /* 微秒级延迟 */

		spin_lock(&srv->lock);
		srv->status = STATUS_EXITING;
		spin_unlock(&srv->lock);

		/* 等待看是否有重新排队 */
		msleep(1100);

		if (srv->echo_count > 1) {
			printk(KERN_WARNING "!!! 竞态条件触发！echo_count=%d\n", srv->echo_count);
		} else {
			printk(KERN_INFO "这次没有触发竞态\n");
		}

		cancel_delayed_work_sync(&srv->echo_work);
		kfree(srv);
	}
}

static void run_test_case_1(void)
{
	struct server_info *srv;

	/* 测试1: 错误的顺序 */
	srv = kzalloc(sizeof(*srv), GFP_KERNEL);
	if (!srv)
		return;
	spin_lock_init(&srv->lock);
	INIT_DELAYED_WORK(&srv->echo_work, echo_request);
	srv->status = STATUS_NORMAL;
	srv->echo_count = 0;

	/* 启动echo工作 */
	queue_delayed_work(system_wq, &srv->echo_work, HZ/2);
	msleep(600); /* 让echo运行一次 */

	put_session_wrong_order(srv);
	cancel_delayed_work_sync(&srv->echo_work); /* 最终清理 */
	printk(KERN_INFO "cancel_delayed_work_sync...\n");

	/* 测试调用 cancel_delayed_work_sync() 之后还会执行echo_request() */
	msleep(10000);
	kfree(srv);
}

static void run_test_case_2(void)
{
	struct server_info *srv;

	/* 测试2: 正确的顺序 */
	srv = kzalloc(sizeof(*srv), GFP_KERNEL);
	if (!srv)
		return;
	spin_lock_init(&srv->lock);
	INIT_DELAYED_WORK(&srv->echo_work, echo_request);
	srv->status = STATUS_NORMAL;
	srv->echo_count = 0;

	/* 启动echo工作 */
	queue_delayed_work(system_wq, &srv->echo_work, HZ/2);
	msleep(600);

	put_session_correct_order(srv);
	kfree(srv);
}

static void run_test_case_3(void)
{
	struct server_info *srv;

	/* 测试3: 使用补丁 */
	srv = kzalloc(sizeof(*srv), GFP_KERNEL);
	if (!srv)
		return;
	spin_lock_init(&srv->lock);
	INIT_DELAYED_WORK(&srv->echo_work, echo_request);
	srv->status = STATUS_NORMAL;
	srv->echo_count = 0;

	/* 启动echo工作 */
	queue_delayed_work(system_wq, &srv->echo_work, HZ/2);
	msleep(600);

	/* 先模拟错误的put_session */
	printk(KERN_INFO "\n[%d] Simulating put_session with wrong order...\n", task_pid_nr(current));
	cancel_delayed_work_sync(&srv->echo_work);
	msleep(50);
	/* 然后clean_demultiplex_info with patch */
	clean_with_patch(srv);
	kfree(srv);
}

static int test_thread_fn(void *data)
{
	printk(KERN_INFO "Test thread started, test_case=%d\n", test_case);

	switch (test_case) {
	case 0: /* 运行所有测试 */
		printk(KERN_INFO "Running all test cases...\n");
		run_test_case_1();
		run_test_case_2();
		run_test_case_3();
		race_demo_test();
		break;
	case 1:
		printk(KERN_INFO "Running test case 1: wrong order\n");
		run_test_case_1();
		break;
	case 2:
		printk(KERN_INFO "Running test case 2: correct order\n");
		run_test_case_2();
		break;
	case 3:
		printk(KERN_INFO "Running test case 3: with patch\n");
		run_test_case_3();
		break;
	case 4:
		printk(KERN_INFO "Running test case 4: race demo\n");
		race_demo_test();
		break;
	default:
		printk(KERN_INFO "test case invalid\n");
		break;
	}

	printk(KERN_INFO "\n=== Test completed ===\n");
	return 0;
}

static int __init delayed_work_race_init(void)
{
	printk(KERN_INFO "delayed_work_race module loaded\n");
	printk(KERN_INFO "This module demonstrates the race condition in delayed_work\n");

	/* 创建测试线程 */
	test_thread = kthread_run(test_thread_fn, NULL, "dwork_test");
	if (IS_ERR(test_thread)) {
		printk(KERN_ERR "Failed to create test thread\n");
		return PTR_ERR(test_thread);
	}

	return 0;
}

static void __exit delayed_work_race_exit(void)
{
	if (test_thread)
		kthread_stop(test_thread);

	printk(KERN_INFO "delayed_work_race module unloaded\n");
}

module_init(delayed_work_race_init);
module_exit(delayed_work_race_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Demonstrate delayed_work race condition similar to CIFS bug");
