# Verify 约束

每次修改代码后，必须执行编译和测试，统一通过 `scripts/verify.py` 触发，不允许跳过。

## 统一入口

- 默认（推荐）：`python scripts/verify.py`
  - 等价于 `bills` 工作流（编译 + CLI 测试）。
- 仅编译 `bills_master`：`python scripts/verify.py bills-build -- build_fast`
- 仅编译 `log_generator`：`python scripts/verify.py log-build -- build --mode Debug`

## 执行规则

- 任何代码修改后，至少执行一次对应的 `verify` 命令并确保返回码为 `0`。
- 若 `verify` 失败，先修复问题，再重复执行，直到通过。
