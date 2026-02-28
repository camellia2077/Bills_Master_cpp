# bills_core Build And Test

## 统一验证入口

- `python scripts/verify.py`

该入口负责编译与 CLI 测试联动，是默认验收命令。

## 测试分层（建议）

- 内部逻辑测试（unit/component）：
  - `python scripts/verify.py logic-tests`
  - 默认包含：`core-build` + `core-abi`
- 结果产物测试（integration/e2e/snapshot）：
  - `python scripts/verify.py artifact-tests`
  - 默认包含：`bills` + `report-consistency-gate`
- 全量测试（内部 + 产物）：
  - `python scripts/verify.py all-tests`

## 目录分组（logic / artifact）

- 测试脚本目录：
  - 逻辑层：`test/suites/logic/bills_core_abi/run_tests.py`
  - 产物层：`test/suites/artifact/bills_master/run_tests.py`
- 测试输出目录：
  - 逻辑层预留：`test/output/logic/`
  - 产物层默认：`test/output/artifact/<project>/`

## 常见子命令

- 并行冒烟（model-first vs json-first）：
  - `python scripts/verify.py bills-parallel-smoke`
  - 作用：并发执行两套 bills 测试并自动做一致性校验（默认对比 `md`）。
  - 可选：
    - `python scripts/verify.py bills-parallel-smoke -- --compare-scope all`
    - `python scripts/verify.py bills-parallel-smoke -- --model-project bills_a --json-project bills_b`
- 仅构建 bills：
  - `python scripts/verify.py bills-build -- build_fast`
- 仅构建 log_generator：
  - `python scripts/verify.py log-build -- build --mode Debug`
- 一致性门禁（含性能阈值）：
  - `python scripts/verify.py report-consistency-gate`
  - 默认格式：`md,tex,typ`
  - 默认性能阈值：`model-first` 相比 `json-first` 不超过 `+10%`

## 测试结果读取（给 agent）

- 汇总 JSON：
  - `test/output/artifact/<project>/test_summary.json`
  - 关键字段：`ok`、`total`、`success`、`failed`
- Python 运行日志：
  - `test/output/artifact/<project>/test_python_output.log`
  - 包含开始时间、结束时间、return code、stdout/stderr。
- 单次运行产物（并行场景）：
  - `test/output/artifact/<project>/runs/<run_id>/`

## 失败排查

- 单步骤日志目录：
  - `test/output/artifact/<project>/logs/`（latest）
  - `test/output/artifact/<project>/runs/<run_id>/logs/`（run 级）
- 先看 `test_summary.json` 判定失败步数，再看对应日志文件。
