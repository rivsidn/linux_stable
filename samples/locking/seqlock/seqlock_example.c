#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/seqlock.h>

static seqcount_t seq_test = SEQCNT_ZERO(seq_test);

static int __init seqlock_demo_init(void)
{
	int i;

	pr_info("%s\n", __func__);

	for (i = 0; i < 3; i++) {
		write_seqcount_begin(&seq_test);
		pr_info("%d\n", seq_test.sequence);
		write_seqcount_end(&seq_test);
		pr_info("%d\n", seq_test.sequence);
	}

	return 0;
}

static void __exit seqlock_demo_exit(void)
{
	pr_info("%s\n", __func__);
	return;
}

module_init(seqlock_demo_init);
module_exit(seqlock_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_DESCRIPTION("seqlock examples");
MODULE_VERSION("1.0");
