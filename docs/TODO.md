
### dev_graft_qdisc() 解释一下这个函数的并发关系

 • dev_graft_qdisc() (net/sched/sch_generic.c:1004) 的作用是：把一个新的根 Qdisc 配置到某个 netdev_queue 上，同时把该 TX 队列的数据路径暂时切换到 noop_qdisc，并返回旧 Qdisc。

  它不是“立即开始使用新 Qdisc”的完整流程，而是 Qdisc 替换过程中的底层一步。

  等价逻辑如下：

  old = dev_queue->qdisc_sleeping;

  lock(old->q.lock);

  if (new == NULL)
      new = &noop_qdisc;

  dev_queue->qdisc_sleeping = new; // 保存新配置
  RCU_ASSIGN(dev_queue->qdisc, &noop_qdisc); // 数据路径暂时不使用旧配置

  unlock(old->q.lock);

  return old;

  两个指针的含义是关键：

  dev_queue->qdisc_sleeping
  = 控制面保存的“应该配置成什么 Qdisc”

  dev_queue->qdisc
  = 发送路径当前实际使用的 Qdisc
  = __dev_queue_xmit() 会经由它处理 skb

  替换过程可以理解为：

  旧 Qdisc 正在服务发送路径
      -> 锁住旧 Qdisc，避免与入队/出队并发冲突
      -> qdisc_sleeping 改为新 Qdisc
      -> qdisc 暂时改为 noop_qdisc
      -> 调用方完成重新激活
      -> dev_activate()
      -> qdisc 被切换为 qdisc_sleeping，即新 Qdisc

  最后一步在 transition_one_qdisc() (net/sched/sch_generic.c:1070)：

  new_qdisc = dev_queue->qdisc_sleeping;
  rcu_assign_pointer(dev_queue->qdisc, new_qdisc);

  所以 dev_graft_qdisc() 的命名中：

  • graft：把新 Qdisc 嫁接到设备 TX 队列。
  • 返回值：返回旧的 qdisc_sleeping，由调用者在切换完成后调用 qdisc_destroy(old) 释放。例如 qdisc_graft() (net/sched/sch_api.c:973) 在替换 root Qdisc 后销毁旧对象。
  • qdisc == NULL：表示卸载该 Qdisc，函数会用 noop_qdisc 代替，保证 dev_queue->qdisc 始终可安全访问。

  spin_lock_bh() 锁的是旧根 Qdisc 的队列锁，同时禁用本 CPU softirq，避免网络发送软中断正在并发访问旧 Qdisc 时发生竞态。
 
- Qdisc netlink 添加，netlink 的注册，netlink 是怎么管理的？


