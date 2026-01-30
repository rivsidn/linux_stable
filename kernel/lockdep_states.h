/*
 * Lockdep states,
 *
 * please update XXX_LOCK_USAGE_STATES in include/linux/lockdep.h whenever
 * you add one, or come up with a nice dynamic solution.
 */
LOCKDEP_STATE(HARDIRQ)
LOCKDEP_STATE(SOFTIRQ)
/*
 * 针对于设置了 __GFP_FS 标识位的进程，执行内存回收操作的时候允许
 * 执行文件系统操作(写回脏页面、释放缓存等).
 * 由于这些操作会获取文件系统相关的锁，极其容易导致死锁问题，所以
 * 这里单独处理.
 */
LOCKDEP_STATE(RECLAIM_FS)
