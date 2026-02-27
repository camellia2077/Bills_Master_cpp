# Verify 约束

每次修改代码后，必须执行编译和测试，统一通过 `scripts/verify.py` 触发，不允许跳过。

## 统一入口

- 默认（推荐）：`python scripts/verify.py`
  - 等价于 `bills` 工作流（编译 + CLI 测试）。
- 并行冒烟（推荐用于流水线一致性）：`python scripts/verify.py bills-parallel-smoke`
  - 并发执行两套：
    - `model-first`
    - `json-first`
  - 自动校验：
    - 两套 `test_summary.json` 都为成功
    - 两套导出产物做项目间快照对比（默认 `md`）
- 一致性门禁（Phase 5.3）：`python scripts/verify.py report-consistency-gate`
  - 默认执行：
    - `formats=md,tex,typ`
    - `compare-scope=all`
    - 性能门禁：`model-first` 相比 `json-first` 回归不超过 `+10%`
- 仅编译 `bills_master`：`python scripts/verify.py bills-build -- build_fast`
- 仅编译 `log_generator`：`python scripts/verify.py log-build -- build --mode Debug`

并行冒烟可选参数：

- `--model-project <name>`：指定 model-first 输出项目名。
- `--json-project <name>`：指定 json-first 输出项目名。
- `--formats <list>`：导出格式，默认 `md`。
- `--compare-scope all|md|json|tex|typ`：对比范围，默认 `md`。
- `--build-dir-mode isolated|shared`：构建目录策略，默认 `isolated`。
- `--enforce-performance-gate`：启用性能回归门禁。
- `--max-performance-regression <ratio>`：最大允许回归比例，默认 `0.10`（+10%）。
- 其他参数会透传到两套 `build_then_cli_test.py`（如 `--single-year` 等）。

## 执行规则

- 任何代码修改后，至少执行一次对应的 `verify` 命令并确保返回码为 `0`。
- 若 `verify` 失败，先修复问题，再重复执行，直到通过。
