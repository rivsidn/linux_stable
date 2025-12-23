/*
 * Generic Netlink 用户空间测试程序
 *
 * 编译:
 *   gcc -Wall -O2 -o generic_netlink_user generic_netlink_user.c
 *
 * 使用:
 *   ./generic_netlink_user "your message"
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/genetlink.h>
#include <linux/netlink.h>

#define GENL_DEMO_FAMILY_NAME	"genl_demo"
#define NL_RECV_BUF		4096

/* 属性定义（需与内核保持一致） */
enum {
	GENL_DEMO_ATTR_UNSPEC,
	GENL_DEMO_ATTR_MSG,
	__GENL_DEMO_ATTR_MAX,
};
#define GENL_DEMO_ATTR_MAX (__GENL_DEMO_ATTR_MAX - 1)

/* 命令定义（需与内核保持一致） */
enum {
	GENL_DEMO_CMD_UNSPEC,
	GENL_DEMO_CMD_ECHO,
	__GENL_DEMO_CMD_MAX,
};
#define GENL_DEMO_CMD_MAX (__GENL_DEMO_CMD_MAX - 1)

struct nl_req {
	struct nlmsghdr nlh;
	struct genlmsghdr gnlh;
	char data[1024];
};

/* Netlink 属性辅助宏 */
#define NLA_DATA(nla)	((void *)((char *)(nla) + NLA_HDRLEN))

static int socket_init(int *sockfd)
{
	struct sockaddr_nl local;
	int fd;

	*sockfd = 0;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = getpid();
	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		close(fd);
		return -1;
	}

	*sockfd = fd;
	return 0;
}

static int add_attr(struct nlmsghdr *nlh, int maxlen, int type,
		    const void *data, int datalen)
{
	struct nlattr *nla;
	int len = NLA_HDRLEN + datalen;
	int total_len = NLMSG_ALIGN(nlh->nlmsg_len);

	if (total_len + NLA_ALIGN(len) > maxlen)
		return -E2BIG;

	nla = (struct nlattr *)((char *)nlh + total_len);
	nla->nla_type = type;
	nla->nla_len = len;
	memcpy(NLA_DATA(nla), data, datalen);
	nlh->nlmsg_len = total_len + NLA_ALIGN(len);

	return 0;
}

static int add_attr_string(struct nlmsghdr *nlh, int maxlen, int type,
			   const char *str)
{
	return add_attr(nlh, maxlen, type, str, strlen(str) + 1);
}

static struct nlattr *find_attr(struct nlattr *attrs, int attrlen, int type)
{
	struct nlattr *nla;
	int len = attrlen;

	for (nla = attrs; NLA_ALIGN(nla->nla_len) <= len;
	     nla = (struct nlattr *)((char *)nla + NLA_ALIGN(nla->nla_len))) {
		if (nla->nla_type == type)
			return nla;
		len -= NLA_ALIGN(nla->nla_len);
		if (len < (int)sizeof(*nla))
			break;
	}

	return NULL;
}

static int get_family_id(int sockfd, uint32_t *seq)
{
	unsigned int family_id;
	static uint32_t seq_num = 1;
	struct sockaddr_nl kernel = {
		.nl_family = AF_NETLINK,
	};
	struct nl_req req;
	char recv_buf[NL_RECV_BUF];
	struct nlmsghdr *nlh;
	struct genlmsghdr *gnlh;
	struct nlattr *nla;
	int len;

	*seq = seq_num++;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	req.nlh.nlmsg_type = GENL_ID_CTRL;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nlh.nlmsg_seq = *seq;
	req.nlh.nlmsg_pid = getpid();

	req.gnlh.cmd = CTRL_CMD_GETFAMILY;
	req.gnlh.version = 1;

	if (add_attr_string(&req.nlh, sizeof(req), CTRL_ATTR_FAMILY_NAME,
			    GENL_DEMO_FAMILY_NAME) < 0) {
		return -1;
	}

	if (sendto(sockfd, &req, req.nlh.nlmsg_len, 0,
		   (struct sockaddr *)&kernel, sizeof(kernel)) < 0) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
	if (len < 0) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__,
			strerror(errno));
		return -1;
	}

	nlh = (struct nlmsghdr *)recv_buf;
	if (!NLMSG_OK(nlh, len) || nlh->nlmsg_seq != *seq) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, "invalid netlink message");
		return -1;
	}

	if (nlh->nlmsg_type == NLMSG_ERROR) {
		struct nlmsgerr *err;

		err = (struct nlmsgerr *)NLMSG_DATA(nlh);
		if (err->error != 0) {
			fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, strerror(-err->error));
			return -1;
		}
	}

	gnlh = (struct genlmsghdr *)NLMSG_DATA(nlh);
	nla = find_attr((struct nlattr *)((char *)gnlh + GENL_HDRLEN),
			nlh->nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN),
			CTRL_ATTR_FAMILY_ID);
	if (!nla) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, "family id not found");
		return -1;
	}

	family_id = *(uint16_t *)NLA_DATA(nla);

	return family_id;
}

static int send_request(int sockfd, int family_id, const char *message,
			uint32_t *seq)
{
	static uint32_t seq_num = 100;
	struct sockaddr_nl kernel = {
		.nl_family = AF_NETLINK,
	};
	struct nl_req req;

	*seq = seq_num++;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	req.nlh.nlmsg_type = family_id;
	req.nlh.nlmsg_flags = NLM_F_REQUEST;
	req.nlh.nlmsg_seq = *seq;
	req.nlh.nlmsg_pid = getpid();

	req.gnlh.cmd = GENL_DEMO_CMD_ECHO;
	req.gnlh.version = 1;

	if (add_attr_string(&req.nlh, sizeof(req), GENL_DEMO_ATTR_MSG,
			    message) < 0) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__,
			"failed to add message");
		return -1;
	}

	if (sendto(sockfd, &req, req.nlh.nlmsg_len, 0,
		   (struct sockaddr *)&kernel, sizeof(kernel)) < 0) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__,
			strerror(errno));
		return -1;
	}

	return 0;
}

static void show_reply(struct nlmsghdr *nlh)
{
	struct genlmsghdr *gnlh = (struct genlmsghdr *)NLMSG_DATA(nlh);
	int attrlen = nlh->nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN);
	struct nlattr *nla;

	nla = find_attr((struct nlattr *)((char *)gnlh + GENL_HDRLEN),
			attrlen, GENL_DEMO_ATTR_MSG);
	if (nla)
		printf("reply: %s\n", (char *)NLA_DATA(nla));
}

static int recv_reply(int sockfd, int family_id, uint32_t seq)
{
	char recv_buf[NL_RECV_BUF];

	while (1) {
		int len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
		struct nlmsghdr *nlh;

		if (len < 0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__,
				strerror(errno));
			return -1;
		}

		for (nlh = (struct nlmsghdr *)recv_buf; NLMSG_OK(nlh, len);
		     nlh = NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_seq != seq)
				continue;

			switch (nlh->nlmsg_type) {
			case NLMSG_DONE:
				return 0;
			case NLMSG_ERROR: {
				struct nlmsgerr *err;

				err = (struct nlmsgerr *)NLMSG_DATA(nlh);
				if (err->error == 0)
					continue;
				fprintf(stderr, "%s %d: %s\n", __FILE__,
					__LINE__, strerror(-err->error));
				return -1;
			}
			default:
				if (nlh->nlmsg_type == (uint16_t)family_id) {
					show_reply(nlh);
					return 0;
				}
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int sockfd;
	int family_id;
	uint32_t seq;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <message>\n", argv[0]);
		return -1;
	}

	if (socket_init(&sockfd) != 0)
		return -1;

	family_id = get_family_id(sockfd, &seq);
	if (family_id < 0) {
		close(sockfd);
		return -1;
	}

	if (send_request(sockfd, family_id, argv[1], &seq) != 0) {
		close(sockfd);
		return -1;
	}

	if (recv_reply(sockfd, family_id, seq) != 0) {
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}
