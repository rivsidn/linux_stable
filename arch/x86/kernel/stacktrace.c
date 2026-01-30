/*
 * Stack trace management functions
 *
 *  Copyright (C) 2006-2009 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 */
#include <linux/sched.h>
#include <linux/stacktrace.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/stacktrace.h>

static void save_stack_warning(void *data, char *msg)
{
}

static void
save_stack_warning_symbol(void *data, char *msg, unsigned long symbol)
{
}

static int save_stack_stack(void *data, char *name)
{
	return 0;
}

static void save_stack_address(void *data, unsigned long addr, int reliable)
{
	struct stack_trace *trace = data;
	if (!reliable)
		return;
	if (trace->skip > 0) {
		trace->skip--;
		return;
	}
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = addr;
}

static void
save_stack_address_nosched(void *data, unsigned long addr, int reliable)
{
	struct stack_trace *trace = (struct stack_trace *)data;
	if (!reliable)
		return;
	if (in_sched_functions(addr))
		return;
	if (trace->skip > 0) {
		trace->skip--;
		return;
	}
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = addr;
}

static const struct stacktrace_ops save_stack_ops = {
	.warning	= save_stack_warning,
	.warning_symbol	= save_stack_warning_symbol,
	.stack		= save_stack_stack,
	.address	= save_stack_address,
	.walk_stack	= print_context_stack,
};

static const struct stacktrace_ops save_stack_ops_nosched = {
	.warning	= save_stack_warning,
	.warning_symbol	= save_stack_warning_symbol,
	.stack		= save_stack_stack,
	.address	= save_stack_address_nosched,
	.walk_stack	= print_context_stack,
};

/*
 * Save stack-backtrace addresses into a stack_trace buffer.
 */
/*
 * 函数: save_stack_trace
 * 追踪对象: 当前进程
 * 栈类型: 内核栈
 * 特殊参数: 无
 * 使用的ops: save_stack_ops
 * ────────────────────────────────────────
 * 函数: save_stack_trace_bp
 * 追踪对象: 当前进程
 * 栈类型: 内核栈
 * 特殊参数: 指定bp起始点
 * 使用的ops: save_stack_ops
 * ────────────────────────────────────────
 * 函数: save_stack_trace_tsk
 * 追踪对象: 指定进程
 * 栈类型: 内核栈
 * 特殊参数: task_struct
 * 使用的ops: save_stack_ops_nosched
 * ────────────────────────────────────────
 * 函数: save_stack_trace_user
 * 追踪对象: 当前进程
 * 栈类型: 用户栈
 * 特殊参数: 无
 * 使用的ops: 专用用户态遍历
 *
 */
void save_stack_trace(struct stack_trace *trace)
{
	dump_trace(current, NULL, NULL, 0, &save_stack_ops, trace);
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}
EXPORT_SYMBOL_GPL(save_stack_trace);

/*
 * bp(base pointer)
 *
 * ● bp 是 Base Pointer（基址指针）的缩写，对应 x86 架构中
 * 的 RBP（64位）或 EBP（32位）寄存器。
 *
 *   栈帧结构：
 *
 *   高地址
 *   ┌─────────────────┐
 *   │   参数 n        │
 *   │   ...           │
 *   │   参数 1        │
 *   ├─────────────────┤
 *   │   返回地址      │  ← [BP + 8] (64位)
 *   ├─────────────────┤
 *   │   上一帧的 BP   │  ← BP 指向这里
 *   ├─────────────────┤
 *   │   局部变量      │
 *   │   ...           │
 *   └─────────────────┘
 *   低地址 (栈顶 SP)
 *
 *   BP 的作用：
 *   - [BP] 保存调用者的 BP 值，形成一个链表
 *   - [BP + 8] 保存返回地址
 *   - 通过不断读取 [BP] 可以遍历整个调用栈
 */
void save_stack_trace_bp(struct stack_trace *trace, unsigned long bp)
{
	dump_trace(current, NULL, NULL, bp, &save_stack_ops, trace);
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}

/* 可以指定进程 */
void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
	dump_trace(tsk, NULL, NULL, 0, &save_stack_ops_nosched, trace);
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}
EXPORT_SYMBOL_GPL(save_stack_trace_tsk);

/* Userspace stacktrace - based on kernel/trace/trace_sysprof.c */

struct stack_frame {
	const void __user	*next_fp;
	unsigned long		ret_addr;
};

static int copy_stack_frame(const void __user *fp, struct stack_frame *frame)
{
	int ret;

	if (!access_ok(VERIFY_READ, fp, sizeof(*frame)))
		return 0;

	ret = 1;
	pagefault_disable();
	if (__copy_from_user_inatomic(frame, fp, sizeof(*frame)))
		ret = 0;
	pagefault_enable();

	return ret;
}

static inline void __save_stack_trace_user(struct stack_trace *trace)
{
	const struct pt_regs *regs = task_pt_regs(current);
	const void __user *fp = (const void __user *)regs->bp;

	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = regs->ip;

	while (trace->nr_entries < trace->max_entries) {
		struct stack_frame frame;

		frame.next_fp = NULL;
		frame.ret_addr = 0;
		if (!copy_stack_frame(fp, &frame))
			break;
		if ((unsigned long)fp < regs->sp)
			break;
		if (frame.ret_addr) {
			trace->entries[trace->nr_entries++] =
				frame.ret_addr;
		}
		if (fp == frame.next_fp)
			break;
		fp = frame.next_fp;
	}
}

/* 采集用户态符号表，传递给用户态进程，用户态进程负责解析 */
void save_stack_trace_user(struct stack_trace *trace)
{
	/*
	 * Trace user stack if we are not a kernel thread
	 */
	if (current->mm) {
		__save_stack_trace_user(trace);
	}
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}

