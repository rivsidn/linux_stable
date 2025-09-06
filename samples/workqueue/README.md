# Linux Workqueue Sample Module

本示例演示了Linux内核工作队列（workqueue）的基本使用方法。

## 功能特性

- 创建自定义工作队列
- 使用普通工作项（work_struct）
- 使用延迟工作项（delayed_work）
- 动态分配工作项
- 工作取消和队列刷新
- 使用系统默认工作队列

## 编译配置

1. 在内核配置中启用示例代码：
```bash
make menuconfig
# Device Drivers -> Sample kernel code -> Build workqueue sample
CONFIG_SAMPLE_WORKQUEUE=m
```

2. 编译模块：
```bash
make M=samples/workqueue
```

## 使用方法

### 加载模块
```bash
insmod samples/workqueue/workqueue_sample.ko
# 或指定延迟参数
insmod samples/workqueue/workqueue_sample.ko delay_seconds=10
```

### 查看输出
```bash
dmesg | grep workqueue_sample
```

### 卸载模块
```bash
rmmod workqueue_sample
```

## 代码说明

### 主要数据结构
- `sample_wq`: 自定义工作队列
- `sample_work`: 普通工作项
- `sample_delayed_work`: 延迟工作项
- `work_data`: 动态工作项数据

### 关键函数
- `create_singlethread_workqueue()`: 创建单线程工作队列
- `queue_work()`: 提交工作到队列
- `queue_delayed_work()`: 提交延迟工作
- `cancel_delayed_work_sync()`: 取消延迟工作
- `flush_workqueue()`: 刷新工作队列
- `destroy_workqueue()`: 销毁工作队列

## 输出示例

```
workqueue_sample: Module loading...
workqueue_sample: Created workqueue 'sample_workqueue'
workqueue_sample: Normal work queued
workqueue_sample: Delayed work queued (delay=5 seconds)
workqueue_sample: Dynamic work #1 queued successfully
workqueue_sample: Executing normal work #1 on CPU 0
workqueue_sample: Dynamic work #1: Dynamic work created at jiffies=xxx (CPU 2)
workqueue_sample: Normal work #1 completed
workqueue_sample: Executing delayed work #2 on CPU 1 (delayed 5 seconds)
...
```

## 注意事项

1. 工作队列在进程上下文执行，可以睡眠
2. 避免在工作函数中长时间占用CPU
3. 动态分配的工作项需要正确管理内存
4. 模块卸载时必须取消所有未完成的工作