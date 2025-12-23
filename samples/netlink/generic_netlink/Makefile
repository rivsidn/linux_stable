# Makefile for generic netlink samples

# 内核模块
obj-$(CONFIG_SAMPLE_NETLINK) += generic_netlink.o

# 用户空间程序
hostprogs-$(CONFIG_SAMPLE_NETLINK) := generic_netlink_user
always := $(hostprogs-y) $(hostprogs-m)
