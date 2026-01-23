/*
 * lockdep.c - 一个简单的Linux内核模块，用于调试lockdep
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/spinlock.h>

static DEFINE_SPINLOCK(static_lock_a);
static DEFINE_SPINLOCK(static_lock_b);

static void dump_lock_class_key(const char *tag, spinlock_t *lock)
{
	spin_lock(lock);
	spin_unlock(lock);

#ifdef CONFIG_DEBUG_LOCK_ALLOC
	pr_info("[%s] lock=%p dep_map=%p name=%s key=%p class=%p%s\n",
		tag, lock, &lock->dep_map, lock->dep_map.name,
		lock->dep_map.key, lock->dep_map.class_cache,
		lock->dep_map.key == (void *)lock ? " (static key)" : "");
#else
	pr_info("[%s] CONFIG_DEBUG_LOCK_ALLOC 未开启，无法输出 lockdep class 信息\n",
		tag);
#endif
}

static int __init basic_demo_init(void)
{
	int i;
	char name[32];
	spinlock_t dynamic_lock_a;
	spinlock_t dynamic_lock_b;
	spinlock_t dynamic_lock_array[10];

	spin_lock_init(&dynamic_lock_a);
	spin_lock_init(&dynamic_lock_b);

	for (i = 0; i < 10; i++) {
		spin_lock_init(&dynamic_lock_array[i]);
	}

	pr_info("Dump lockdep class key 信息\n");

	dump_lock_class_key("static_lock_a", &static_lock_a);
	dump_lock_class_key("static_lock_b", &static_lock_b);

	dump_lock_class_key("dynamic_lock_a", &dynamic_lock_a);
	dump_lock_class_key("dynamic_lock_b", &dynamic_lock_b);

	for (i = 0; i < ARRAY_SIZE(dynamic_lock_array); i++) {
		snprintf(name, sizeof(name), "dynamic_lock_array[%d]", i);
		dump_lock_class_key(name, &dynamic_lock_array[i]);
	}

	return 0;
}

static void __exit basic_demo_exit(void)
{
	return;
}

module_init(basic_demo_init);
module_exit(basic_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("A simple kernel module example");
MODULE_VERSION("1.0");
