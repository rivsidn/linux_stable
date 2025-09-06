/*
 * workqueue_sample.c - 工作队列示例内核模块
 *
 * 演示Linux内核工作队列的基本使用方法：
 * 1. 创建和销毁工作队列
 * 2. 初始化工作项
 * 3. 提交普通工作和延迟工作
 * 4. 取消和刷新工作队列
 *
 * 模块参数：
 * - sample_mode: 示例模式选择
 *   0 = 所有示例（默认）
 *   1 = 仅普通工作项
 *   2 = 仅延迟工作项
 *   3 = 仅动态工作项
 *   4 = 仅系统工作队列
 * - delay_seconds: 延迟工作的延迟时间（1-60秒，默认5秒）
 * - dynamic_work_count: 动态工作项数量（1-10个，默认3个）
 *
 * 使用方法：
 * insmod workqueue_sample.ko sample_mode=1 delay_seconds=10
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sample Author");
MODULE_DESCRIPTION("Linux Workqueue Sample Module");

/* 工作队列相关变量 */
static struct workqueue_struct *sample_wq;
static struct work_struct sample_work;
static struct delayed_work sample_delayed_work;

/* 工作项计数器 */
static atomic_t work_count = ATOMIC_INIT(0);

/* 模块参数：延迟时间（秒） */
static int delay_seconds = 5;
module_param(delay_seconds, int, 0644);
MODULE_PARM_DESC(delay_seconds, "Delay time in seconds for delayed work (default=5)");

/* 模块参数：示例模式 */
static int sample_mode = 0;
module_param(sample_mode, int, 0644);
MODULE_PARM_DESC(sample_mode, "Sample mode: 0=all(default), 1=normal_only, 2=delayed_only, 3=dynamic_only, 4=system_wq_only");

/* 模块参数：动态工作项数量 */
static int dynamic_work_count = 3;
module_param(dynamic_work_count, int, 0644);
MODULE_PARM_DESC(dynamic_work_count, "Number of dynamic work items to create (default=3)");

/* 工作数据结构 */
struct work_data {
	int id;
	char message[64];
	struct work_struct work;
};

/* 普通工作处理函数 */
static void sample_work_handler(struct work_struct *work)
{
	int count = atomic_inc_return(&work_count);
	
	pr_info("workqueue_sample: Executing normal work #%d on CPU %d\n",
		count, smp_processor_id());
	
	/* 模拟工作负载 */
	msleep(100);
	
	pr_info("workqueue_sample: Normal work #%d completed\n", count);
}

/* 延迟工作处理函数 */
static void sample_delayed_work_handler(struct work_struct *work)
{
	int count = atomic_inc_return(&work_count);
	struct delayed_work *dwork = to_delayed_work(work);
	
	pr_info("workqueue_sample: Executing delayed work #%d on CPU %d (delayed %d seconds)\n",
		count, smp_processor_id(), delay_seconds);
	
	/* 模拟工作负载 */
	msleep(200);
	
	pr_info("workqueue_sample: Delayed work #%d completed\n", count);
	
	/* 重新调度延迟工作（循环执行） */
	if (count < 3) {
		pr_info("workqueue_sample: Rescheduling delayed work\n");
		queue_delayed_work(sample_wq, dwork, delay_seconds * HZ);
	}
}

/* 动态分配的工作处理函数 */
static void sample_dynamic_work_handler(struct work_struct *work)
{
	struct work_data *data = container_of(work, struct work_data, work);
	
	pr_info("workqueue_sample: Dynamic work #%d: %s (CPU %d)\n",
		data->id, data->message, smp_processor_id());
	
	/* 模拟工作负载 */
	msleep(50);
	
	/* 释放动态分配的内存 */
	kfree(data);
}

/* 创建并提交动态工作项 */
static void create_dynamic_work(int id)
{
	struct work_data *data;
	
	data = kmalloc(sizeof(struct work_data), GFP_KERNEL);
	if (!data) {
		pr_err("workqueue_sample: Failed to allocate work data\n");
		return;
	}
	
	data->id = id;
	snprintf(data->message, sizeof(data->message), 
		 "Dynamic work created at jiffies=%lu", jiffies);
	
	INIT_WORK(&data->work, sample_dynamic_work_handler);
	
	if (queue_work(sample_wq, &data->work))
		pr_info("workqueue_sample: Dynamic work #%d queued successfully\n", id);
	else {
		pr_err("workqueue_sample: Failed to queue dynamic work #%d\n", id);
		kfree(data);
	}
}

/* 示例模式：仅普通工作 */
static int run_normal_work_sample(void)
{
	pr_info("workqueue_sample: Running normal work sample only\n");
	
	if (queue_work(sample_wq, &sample_work))
		pr_info("workqueue_sample: Normal work queued\n");
	
	return 0;
}

/* 示例模式：仅延迟工作 */
static int run_delayed_work_sample(void)
{
	pr_info("workqueue_sample: Running delayed work sample only\n");
	
	if (queue_delayed_work(sample_wq, &sample_delayed_work, delay_seconds * HZ))
		pr_info("workqueue_sample: Delayed work queued (delay=%d seconds)\n", 
			delay_seconds);
	
	return 0;
}

/* 示例模式：仅动态工作项 */
static int run_dynamic_work_sample(void)
{
	int i;
	
	pr_info("workqueue_sample: Running dynamic work sample only\n");
	
	for (i = 1; i <= dynamic_work_count; i++) {
		create_dynamic_work(i);
	}
	
	return 0;
}

/* 示例模式：仅系统工作队列 */
static int run_system_workqueue_sample(void)
{
	pr_info("workqueue_sample: Running system workqueue sample only\n");
	
	schedule_work(&sample_work);
	pr_info("workqueue_sample: Work scheduled on system workqueue\n");
	
	return 0;
}

/* 示例模式：全部示例（默认） */
static int run_all_samples(void)
{
	int i;
	
	pr_info("workqueue_sample: Running all workqueue samples\n");
	
	/* 提交普通工作 */
	if (queue_work(sample_wq, &sample_work))
		pr_info("workqueue_sample: Normal work queued\n");
	
	/* 提交延迟工作 */
	if (queue_delayed_work(sample_wq, &sample_delayed_work, delay_seconds * HZ))
		pr_info("workqueue_sample: Delayed work queued (delay=%d seconds)\n", 
			delay_seconds);
	
	/* 创建多个动态工作项 */
	for (i = 1; i <= dynamic_work_count; i++) {
		create_dynamic_work(i);
	}
	
	/* 使用系统工作队列提交工作 */
	schedule_work(&sample_work);
	pr_info("workqueue_sample: Work also scheduled on system workqueue\n");
	
	return 0;
}

/* 模块初始化函数 */
static int __init workqueue_sample_init(void)
{
	int ret;
	
	pr_info("workqueue_sample: Module loading with sample_mode=%d\n", sample_mode);
	
	/* 验证模块参数 */
	if (sample_mode < 0 || sample_mode > 4) {
		pr_err("workqueue_sample: Invalid sample_mode=%d (valid range: 0-4)\n", sample_mode);
		return -EINVAL;
	}
	
	if (dynamic_work_count < 1 || dynamic_work_count > 10) {
		pr_err("workqueue_sample: Invalid dynamic_work_count=%d (valid range: 1-10)\n", dynamic_work_count);
		return -EINVAL;
	}
	
	if (delay_seconds < 1 || delay_seconds > 60) {
		pr_err("workqueue_sample: Invalid delay_seconds=%d (valid range: 1-60)\n", delay_seconds);
		return -EINVAL;
	}
	
	/* 对于系统工作队列模式，不需要创建自己的工作队列 */
	if (sample_mode != 4) {
		/* 创建单线程工作队列 */
		sample_wq = create_singlethread_workqueue("sample_workqueue");
		if (!sample_wq) {
			pr_err("workqueue_sample: Failed to create workqueue\n");
			return -ENOMEM;
		}
		pr_info("workqueue_sample: Created workqueue 'sample_workqueue'\n");
	}
	
	/* 初始化工作项 */
	INIT_WORK(&sample_work, sample_work_handler);
	INIT_DELAYED_WORK(&sample_delayed_work, sample_delayed_work_handler);
	
	/* 根据示例模式执行不同的示例 */
	switch (sample_mode) {
	case 0:
		ret = run_all_samples();
		break;
	case 1:
		ret = run_normal_work_sample();
		break;
	case 2:
		ret = run_delayed_work_sample();
		break;
	case 3:
		ret = run_dynamic_work_sample();
		break;
	case 4:
		ret = run_system_workqueue_sample();
		break;
	default:
		pr_err("workqueue_sample: Unknown sample_mode=%d\n", sample_mode);
		ret = -EINVAL;
		goto cleanup_wq;
	}
	
	if (ret) {
		pr_err("workqueue_sample: Failed to run sample mode %d\n", sample_mode);
		goto cleanup_wq;
	}
	
	pr_info("workqueue_sample: Module loaded successfully\n");
	return 0;

cleanup_wq:
	if (sample_wq) {
		destroy_workqueue(sample_wq);
		sample_wq = NULL;
	}
	return ret;
}

/* 模块卸载函数 */
static void __exit workqueue_sample_exit(void)
{
	pr_info("workqueue_sample: Module unloading...\n");
	
	/* 取消延迟工作 */
	if (sample_mode != 4 && cancel_delayed_work_sync(&sample_delayed_work))
		pr_info("workqueue_sample: Delayed work cancelled\n");
	
	/* 刷新和销毁工作队列 */
	if (sample_wq) {
		/* 刷新工作队列，等待所有工作完成 */
		flush_workqueue(sample_wq);
		pr_info("workqueue_sample: Workqueue flushed\n");
		
		/* 销毁工作队列 */
		destroy_workqueue(sample_wq);
		pr_info("workqueue_sample: Workqueue destroyed\n");
	}
	
	/* 确保系统工作队列中的工作也完成 */
	flush_scheduled_work();
	
	pr_info("workqueue_sample: Total works executed: %d\n", 
		atomic_read(&work_count));
	pr_info("workqueue_sample: Module unloaded\n");
}

module_init(workqueue_sample_init);
module_exit(workqueue_sample_exit);
