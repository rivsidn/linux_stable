# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 概述

当前目录为 Linux 4.4.155 内核源码树。

## 常用构建命令

```bash
# 配置内核
make menuconfig          # 文本菜单配置界面
make defconfig           # 使用默认配置
make oldconfig           # 基于现有 .config 更新配置

# 构建内核
make                     # 构建 vmlinux 和模块
make -j$(nproc)          # 并行构建
make V=1                 # 显示详细编译命令

# 构建单个文件/目录
make dir/file.o          # 构建单个目标文件
make dir/                # 构建指定目录

# 构建外部模块
make M=/path/to/module   # 在指定目录构建模块

# 清理
make clean               # 删除大部分生成文件，保留 .config
make mrproper            # 删除所有生成文件包括 .config
make distclean           # mrproper + 删除编辑器备份文件

# 代码索引 (已生成 cscope 和 tags)
make tags                # 生成 tags 文件
make cscope              # 生成 cscope 索引

# 代码检查
make C=1                 # 对重新编译的文件运行 sparse 检查
make C=2                 # 对所有源文件运行 sparse 检查
scripts/checkpatch.pl    # 检查补丁或文件的编码风格
```

## 内核代码架构

```
arch/           # 架构相关代码 (x86, arm, arm64 等)
block/          # 块设备层
crypto/         # 加密 API
drivers/        # 设备驱动
fs/             # 文件系统
include/        # 头文件
init/           # 内核初始化代码
ipc/            # 进程间通信
kernel/         # 核心子系统 (调度、信号、时间等)
lib/            # 通用库函数
mm/             # 内存管理
net/            # 网络协议栈
samples/        # 示例代码
scripts/        # 构建脚本和辅助工具
security/       # 安全框架 (SELinux, AppArmor 等)
sound/          # 音频子系统
virt/           # 虚拟化支持
```

## 内核编码规范要点

- 缩进使用 Tab (8 字符宽度)
- 行宽限制 80 列
- 函数开括号独占一行，其他控制结构开括号在行尾
- 使用 C89 风格注释 `/* ... */`，避免 C99 风格 `//`
- 局部变量名简短，全局变量/函数名需描述性
- 避免使用 typedef 定义结构体和指针类型
- 函数应短小精悍，局部变量不超过 5-10 个
- 使用 goto 进行集中式函数退出和资源清理
- 使用 `kmalloc(sizeof(*p), ...)` 而非 `kmalloc(sizeof(struct foo), ...)`

详细规范参见 `Documentation/CodingStyle`。

## 交叉编译

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## 调试相关

- 使用 `pr_debug()` / `dev_dbg()` 输出调试信息
- 配置 `CONFIG_DEBUG_INFO` 生成调试符号
- 配置 `CONFIG_KALLSYMS` 保留符号表用于 oops 分析
- `scripts/decode_stacktrace.sh` 解析内核栈回溯
