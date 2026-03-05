# bills_core Build And Test

改动定位建议先看：`docs/modules/bills_core/change_guide.md`

## 统一验证入口

- `python tools/verify/verify.py`

该入口负责编译与 CLI 测试联动，是默认验收命令。

## 分层约束检查（Phase 3.3）

- 目标约束：
  - core 不得依赖 platform/windows；
  - cli 不得新增业务规则实现。
- 快速检查命令（仓库根目录）：
  - `rg -n "windows/" "libs/bills_core/src"`
  - `rg -n "LoadLibrary|GetProcAddress|sqlite3_" "libs/bills_core/src"`
- 结果判定：
  - 命中需人工复核；若属于真实平台依赖，必须迁回 `apps/bills_cli` 适配层。

## 测试分层（建议）

- 内部逻辑测试（unit/component）：
  - `python tools/verify/verify.py logic-tests`
  - 默认包含：`core-build` + `core-abi`
- 结果产物测试（integration/e2e/snapshot）：
  - `python tools/verify/verify.py artifact-tests`
  - 默认包含：`bills` + `report-consistency-gate`
- 全量测试（内部 + 产物）：
  - `python tools/verify/verify.py all-tests`
  - 默认为开发模式：不阻塞性能门禁，仅做一致性校验。
  - 包含模块模式双通道检查：`BILLS_ENABLE_MODULES=OFF/ON`。
  - 若需要阻塞性能门禁：
    - `python tools/verify/verify.py artifact-tests -- --scope full --enforce-performance-gate`
  - Phase 3 验收建议使用该命令（开发期可先不阻塞性能阈值）。

## 目录分组（logic / artifact）

- 测试脚本目录：
  - 逻辑层：`tests/suites/logic/bills_core_abi/run_tests.py`
  - 产物层：`tests/suites/artifact/bills_master/run_tests.py`
- 测试输出目录：
  - 逻辑层预留：`tests/output/logic/`
  - 产物层默认：`tests/output/artifact/<project>/`

## 常见子命令

- 并行冒烟（model-first vs json-first）：
  - `python tools/verify/verify.py bills-parallel-smoke`
  - 作用：并发执行两套 bills 测试并自动做一致性校验（默认对比 `md`）。
  - 可选：
    - `python tools/verify/verify.py bills-parallel-smoke -- --compare-scope all`
    - `python tools/verify/verify.py bills-parallel-smoke -- --model-project bills_a --json-project bills_b`
- 仅构建 bills：
  - `python tools/verify/verify.py bills-build -- build_fast`
- 仅构建 log_generator：
  - `python tools/verify/verify.py log-build -- build --mode Debug`
- 模块模式双通道检查：
  - `python tools/verify/verify.py module-mode-check`
  - 可选参数示例：
    - `python tools/verify/verify.py module-mode-check -- --command build`
    - `python tools/verify/verify.py module-mode-check -- --compiler clang`
- tools 分层约束检查：
  - `python tools/verify/verify.py tools-layer-check`
- 一致性门禁（含性能阈值）：
  - `python tools/verify/verify.py report-consistency-gate`
  - 默认格式：`md,tex,typ`
  - 默认性能阈值：`model-first` 相比 `json-first` 不超过 `+10%`

## 测试结果读取（给 agent）

- 汇总 JSON：
  - `tests/output/artifact/<project>/test_summary.json`
  - 关键字段：`ok`、`total`、`success`、`failed`
- Python 运行日志：
  - `tests/output/artifact/<project>/test_python_output.log`
  - 包含开始时间、结束时间、return code、stdout/stderr。
- 单次运行产物（并行场景）：
  - `tests/output/artifact/<project>/runs/<run_id>/`

## 失败排查

- 单步骤日志目录：
  - `tests/output/artifact/<project>/logs/`（latest）
  - `tests/output/artifact/<project>/runs/<run_id>/logs/`（run 级）
- 先看 `test_summary.json` 判定失败步数，再看对应日志文件。
