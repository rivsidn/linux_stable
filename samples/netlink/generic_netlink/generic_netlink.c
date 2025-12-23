/*
 * Generic Netlink 示例模块
 *
 * 该模块演示了如何使用 Generic Netlink 实现内核与用户空间的通信
 *
 * 功能：
 * - 注册 Generic Netlink family
 * - 接收来自用户空间的消息
 * - 向用户空间发送回复
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/genetlink.h>

/* Generic Netlink family 名称 */
#define GENL_DEMO_FAMILY_NAME		"genl_demo"
#define GENL_DEMO_VERSION		1

/* 属性定义 */
enum {
	GENL_DEMO_ATTR_UNSPEC,
	GENL_DEMO_ATTR_MSG,		/* 消息属性 */
	__GENL_DEMO_ATTR_MAX,
};

#define GENL_DEMO_ATTR_MAX (__GENL_DEMO_ATTR_MAX - 1)

/* 命令定义 */
enum {
	GENL_DEMO_CMD_UNSPEC,
	GENL_DEMO_CMD_ECHO,		/* echo 命令 */
	__GENL_DEMO_CMD_MAX,
};
#define GENL_DEMO_CMD_MAX (__GENL_DEMO_CMD_MAX - 1)

/* 属性策略 */
static struct nla_policy genl_demo_policy[GENL_DEMO_ATTR_MAX + 1] = {
	[GENL_DEMO_ATTR_MSG] = { .type = NLA_NUL_STRING, .len = 256 },
};

/* Generic Netlink family 定义 */
static struct genl_family genl_demo_family = {
	.id		= GENL_ID_GENERATE,
	.hdrsize	= 0,
	.name		= GENL_DEMO_FAMILY_NAME,
	.version	= GENL_DEMO_VERSION,
	.maxattr	= GENL_DEMO_ATTR_MAX,
};

/*
 * echo 命令处理函数
 * 接收用户空间发送的消息，并原样回复
 */
static int genl_demo_echo(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *reply_skb;
	void *msg_head;
	char *msg;
	int ret;

	/* 检查是否包含必需的属性 */
	if (!info->attrs[GENL_DEMO_ATTR_MSG]) {
		pr_err("genl_demo: missing message attribute\n");
		return -EINVAL;
	}

	/* 获取用户空间发送的消息 */
	msg = nla_data(info->attrs[GENL_DEMO_ATTR_MSG]);
	pr_info("genl_demo: received message: %s\n", msg);

	/* 分配回复消息的 skb */
	reply_skb = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!reply_skb) {
		pr_err("genl_demo: failed to allocate reply skb\n");
		return -ENOMEM;
	}

	/* 构建回复消息头 */
	msg_head = genlmsg_put(reply_skb, info->snd_portid, info->snd_seq,
			       &genl_demo_family, 0, GENL_DEMO_CMD_ECHO);
	if (!msg_head) {
		pr_err("genl_demo: failed to put message header\n");
		ret = -ENOMEM;
		goto err_free_skb;
	}

	/* 添加消息属性 */
	ret = nla_put_string(reply_skb, GENL_DEMO_ATTR_MSG, msg);
	if (ret) {
		pr_err("genl_demo: failed to put attribute\n");
		goto err_free_skb;
	}

	/* 完成消息构建 */
	genlmsg_end(reply_skb, msg_head);

	/* 发送回复 */
	ret = genlmsg_reply(reply_skb, info);
	if (ret) {
		pr_err("genl_demo: failed to send reply\n");
		return ret;
	}

	pr_info("genl_demo: reply sent successfully\n");
	return 0;

err_free_skb:
	nlmsg_free(reply_skb);
	return ret;
}

static struct genl_ops genl_demo_ops[] = {
	{
		.cmd	= GENL_DEMO_CMD_ECHO,
		.flags	= 0,
		.policy	= genl_demo_policy,
		.doit	= genl_demo_echo,
		.dumpit	= NULL,
	},
};

static int __init genl_demo_init(void)
{
	int ret;

	ret = genl_register_family_with_ops(&genl_demo_family, genl_demo_ops);
	if (ret) {
		pr_err("genl_demo init failed");
		return ret;
	}

	pr_info("genl_demo init\n");
	return 0;
}

static void __exit genl_demo_exit(void)
{
	genl_unregister_family(&genl_demo_family);
	pr_info("genl_demo exit\n");
}

module_init(genl_demo_init);
module_exit(genl_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netlink Demo");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Generic Netlink example module");
