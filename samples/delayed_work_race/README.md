# Delayed Work Race Condition 示例

这个示例模块演示了CIFS中的delayed_work竞态条件问题。

## 问题描述

在原始的CIFS代码中，存在一个竞态条件：

1. `cifs_put_tcp_session`先调用`cancel_delayed_work_sync()`（第2163行）
2. 然后设置`server->tcpStatus = CifsExiting`（第2179行）

如果在这两个操作之间，`cifs_echo_request`刚好被调度执行：
- 它检查状态时还不是`CifsExiting`
- 所以会继续执行并重新排队自己
- 导致即使调用了`cancel_delayed_work_sync`，工作仍然被重新排队

## 编译和测试

```bash
# 编译模块
cd /path/to/linux-source
make CONFIG_SAMPLE_DELAYED_WORK_RACE=m M=samples/delayed_work_race

# 加载模块（默认运行所有测试）
sudo insmod samples/delayed_work_race/delayed_work_race.ko

# 或指定特定测试用例
sudo insmod samples/delayed_work_race/delayed_work_race.ko test_case=1  # 只测试错误顺序
sudo insmod samples/delayed_work_race/delayed_work_race.ko test_case=2  # 只测试正确顺序
sudo insmod samples/delayed_work_race/delayed_work_race.ko test_case=3  # 只测试补丁方案
sudo insmod samples/delayed_work_race/delayed_work_race.ko test_case=4  # 竞态条件演示

# 查看输出
dmesg | tail -50

# 卸载模块
sudo rmmod delayed_work_race
```

## 模块参数说明

- `test_case`: 控制运行哪个测试用例
  - `0`: 运行所有测试（默认）
  - `1`: 只运行错误顺序测试（模拟原始CIFS问题）
  - `2`: 只运行正确顺序测试（建议的修复方案）
  - `3`: 只运行补丁测试（实际的修复方案）
  - `4`: 竞态条件演示（多次尝试触发竞态）

## 测试输出说明

根据选择的测试用例，模块会输出不同的结果：

1. **错误的顺序**（模拟原始代码）
   - 先cancel_delayed_work_sync
   - 后设置STATUS_EXITING
   - 结果：echo可能继续运行

2. **正确的顺序**（建议的修复）
   - 先设置STATUS_EXITING
   - 后cancel_delayed_work_sync
   - 结果：echo被正确停止

3. **使用补丁**（实际的修复方案）
   - 在clean_demultiplex_info中再次cancel
   - 即使有竞态也能确保安全

4. **竞态条件演示**（test_case=4）
   - 使用更短的延迟和多次尝试
   - 更容易重现竞态条件
   - 展示问题的不确定性

## 关键代码对应关系

- `echo_request()` → `cifs_echo_request()`
- `put_session_wrong_order()` → `cifs_put_tcp_session()`
- `clean_with_patch()` → `clean_demultiplex_info()` with patch
- `STATUS_EXITING` → `CifsExiting`