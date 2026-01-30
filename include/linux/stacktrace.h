#ifndef __LINUX_STACKTRACE_H
#define __LINUX_STACKTRACE_H

struct task_struct;

#ifdef CONFIG_STACKTRACE
struct task_struct;

/*
 * 字段: max_entries
 * 类型: 输入
 * 功能: 数组容量，最多保存多少个栈帧
 * ────────────────────────────────────────
 * 字段: nr_entries
 * 类型: 输出
 * 功能: 实际采集到的栈帧数量
 * ────────────────────────────────────────
 * 字段: entries
 * 类型: 输入/输出
 * 功能: 指向存储返回地址的数组
 * ────────────────────────────────────────
 * 字段: skip
 * 类型: 输入
 * 功能: 跳过栈顶的 N 帧（不记录）
 *
 * 需要自己指定调用栈存储的位置，以及存储多少调用栈.
 */
struct stack_trace {
	unsigned int nr_entries, max_entries;
	unsigned long *entries;
	int skip;	/* input argument: How many entries to skip */
};

extern void save_stack_trace(struct stack_trace *trace);
extern void save_stack_trace_bp(struct stack_trace *trace, unsigned long bp);
extern void save_stack_trace_tsk(struct task_struct *tsk,
				struct stack_trace *trace);

extern void print_stack_trace(struct stack_trace *trace, int spaces);

#ifdef CONFIG_USER_STACKTRACE_SUPPORT
extern void save_stack_trace_user(struct stack_trace *trace);
#else
# define save_stack_trace_user(trace)              do { } while (0)
#endif

#else
# define save_stack_trace(trace)			do { } while (0)
# define save_stack_trace_tsk(tsk, trace)		do { } while (0)
# define save_stack_trace_user(trace)			do { } while (0)
# define print_stack_trace(trace, spaces)		do { } while (0)
#endif

#endif
