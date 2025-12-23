/*
 * 通过 netlink (NETLINK_ROUTE) 获取系统中所有网络接口的简单示例
 *
 * 编译:
 *   gcc -Wall -O2 -o rtnetlink_demo rtnetlink_demo.c
 */

#include <errno.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

//#define DEBUG

#define NL_RECV_BUF 8192

struct nl_req {
	struct nlmsghdr nlh;
	struct ifinfomsg ifm;
};

static void show_socket_addr(int sockfd)
{
#ifdef DEBUG
	struct sockaddr_nl local_addr;
	socklen_t local_addr_len = sizeof(local_addr);

	if (getsockname(sockfd, (struct sockaddr *)&local_addr,
			&local_addr_len) != 0) {
		perror("getsockname");
		return;
	}
	printf("nl_family:\t%d\n", local_addr.nl_family);
	printf("nl_pid:\t\t%u\n", local_addr.nl_pid);
	printf("nl_groups:\t%u\n", local_addr.nl_groups);
#else
	return;
#endif
}

static int socket_init(int *sockfd)
{
	struct sockaddr_nl local;
	int fd;

	*sockfd = 0;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
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

	show_socket_addr(fd);

	*sockfd = fd;

	return 0;
}

static int send_request(int sockfd, uint32_t *seq)
{
	static uint32_t seq_num = 1;
	struct sockaddr_nl kernel = {
		.nl_family = AF_NETLINK,
	};
	struct nl_req req;

	*seq = seq_num++;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	//设置处理的消息类型
	req.nlh.nlmsg_type = RTM_GETLINK;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.nlh.nlmsg_seq = *seq;
	req.nlh.nlmsg_pid = getpid();
	//设置协议类型
	req.ifm.ifi_family = PF_UNSPEC;

	if (sendto(sockfd, &req, req.nlh.nlmsg_len, 0,
		   (struct sockaddr *)&kernel, sizeof(kernel)) < 0) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	return 0;
}

static void show_interfaces(struct nlmsghdr *nlh)
{
	struct ifinfomsg *ifm = NLMSG_DATA(nlh);
	int attr_len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifm));
	struct rtattr *attr;
	const char *ifname = NULL;

	for (attr = IFLA_RTA(ifm); RTA_OK(attr, attr_len);
	     attr = RTA_NEXT(attr, attr_len)) {
		switch (attr->rta_type) {
		case IFLA_IFNAME:
			ifname = (const char *)RTA_DATA(attr);
			break;
		default:
			break;
		}
	}

	printf("%d: %s\n", ifm->ifi_index, ifname ? ifname : "(unknown)");
}

static int recv_reply(int sockfd, uint32_t seq)
{
	char recv_buf[NL_RECV_BUF];

	while (1) {
		int len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
		struct nlmsghdr *nlh;

		if (len < 0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, strerror(errno));
			return -1;
		}

		// 一次recv buff 中可能包含多个netlink msg
		for (nlh = (struct nlmsghdr *)recv_buf; NLMSG_OK(nlh, len);
		     nlh = NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_seq != seq)
				continue;

			switch (nlh->nlmsg_type) {
			case NLMSG_DONE:
				return 0;
			case NLMSG_ERROR: {
				fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__,
					"netlink error");
				return -1;
			}
			case RTM_NEWLINK:
				show_interfaces(nlh);
				break;
			default:
				break;
			}
		}
	}
}

int main(void)
{
	int ret;
	int sockfd;
	uint32_t seq;

	if (socket_init(&sockfd) != 0)
		return -1;

	ret = send_request(sockfd, &seq);
	if (ret != 0) {
		close(sockfd);
		return -1;
	}

	ret = recv_reply(sockfd, seq);
	if (ret != 0) {
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}

