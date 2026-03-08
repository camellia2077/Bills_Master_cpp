# bills_core Dist And Test

改动定位建议先看：`docs/modules/bills_core/change_guide.md`

## 统一入口

- `python tools/verify/verify.py`
- `python tools/run.py dist <target>`

默认验收依旧以 `verify.py` 为主；`dist` 入口负责把 CMake 产物统一落到 `dist/cmake/`。

## 分层约束检查（Phase 3.3）

- 目标约束：
  - core 不得依赖 platform/windows。
  - cli 不得新增业务规则实现。
- 快速检查命令（仓库根目录）：
  - `rg -n "windows/" "libs/bills_core/src"`
  - `rg -n "LoadLibrary|GetProcAddress|sqlite3_" "libs/bills_core/src"`
- 结果判定：
  - 命中需人工复核；若属于真实平台依赖，必须迁回 `apps/bills_cli` 适配层。

## 测试分层（建议）

- 内部逻辑测试（unit/component）：
  - `python tools/verify/verify.py logic-tests`
  - 默认包含：`core-dist` + `core-abi`
- 结果产物测试（integration/e2e/snapshot）：
  - `python tools/verify/verify.py artifact-tests`
  - 默认包含：`bills` + `report-consistency-gate`
- 全量测试（内部 + 产物）：
  - `python tools/verify/verify.py all-tests`
  - 默认为开发模式：不阻塞性能门禁，仅做一致性校验。
  - 包含模块模式双通道检查：`BILLS_ENABLE_MODULES=OFF/ON`。
  - 若需要阻塞性能门禁：
    - `python tools/verify/verify.py artifact-tests -- --scope full --enforce-performance-gate`

## 目录分组（logic / artifact）

- 测试脚本目录：
  - 逻辑层：`tests/suites/logic/bills_core_abi/run_tests.py`
  - 产物层：`tests/suites/artifact/bills_master/run_tests.py`
- 共享输入数据：
  - `testdata/bills/`
- 测试输出目录：
  - 运行时层：`dist/tests/runtime/<project>/`
    - `workspace/`（exe/dll/config）
    - `runs/<run_id>/`（每次执行独立运行目录）
  - 逻辑层：`dist/tests/logic/`
  - 产物层：`dist/tests/artifact/<project>/`

## 常见子命令

- 并行冒烟（model-first vs json-first）：
  - `python tools/verify/verify.py bills-parallel-smoke`
  - 可选：
    - `python tools/verify/verify.py bills-parallel-smoke -- --compare-scope all`
    - `python tools/verify/verify.py bills-parallel-smoke -- --model-project bills_a --json-project bills_b`
- 仅准备 bills CLI：
  - `python tools/verify/verify.py bills-dist`
- 仅准备 `bills_core`：
  - `python tools/verify/verify.py core-dist`
- 仅准备 log_generator：
  - `python tools/verify/verify.py log-dist`
  - 若需要显式走底层 flow：
    - `python tools/verify/verify.py log-dist -- dist --preset debug`
  - 默认 TOML pipeline：`tools/verify/pipelines/log_generator_dist.toml`
- log_generator 完整命令行测试：
  - `python tools/verify/verify.py log-cli-test`
  - 默认 TOML pipeline：`tools/verify/pipelines/log_generator_cli.toml`
- log_generator 运行时配置默认位于：
  - `dist/cmake/log_generator/debug/shared/bin/config/config.toml`
- 生成 log_generator 测试输入（默认落盘到 artifact）：
  - `python tools/flows/log_generator_flow.py generate --preset debug --start-year 2024 --end-year 2024`
  - 生成命令会将 `config.toml` 复制到独立 runtime 目录后再执行生成器。
- 显式将 log_generator 数据提升到共享 testdata：
  - `python tools/flows/log_generator_flow.py promote-testdata`
  - 执行记录：`dist/tests/artifact/log_generator/last_promote.json`
- 模块模式双通道检查：
  - `python tools/verify/verify.py module-mode-check`
  - 可选参数示例：
    - `python tools/verify/verify.py module-mode-check -- --preset release`
    - `python tools/verify/verify.py module-mode-check -- --compiler clang`
- tools 分层约束检查：
  - `python tools/verify/verify.py tools-layer-check`
- 一致性门禁（含性能阈值）：
  - `python tools/verify/verify.py report-consistency-gate`
  - 默认格式：`md,json,tex`
  - 默认性能阈值：`model-first` 相比 `json-first` 不超过 `+10%`
- TOML 流程 Runner：
  - 通用入口：`python tools/verify/verify.py pipeline-run -- --config tools/verify/pipelines/bills_artifact.toml`
  - bills 示例：`python tools/verify/verify.py pipeline-bills`
  - log_generator 示例：`python tools/verify/verify.py pipeline-log-generator`
- reporting workflow（独立于核心门禁）：
  - compile2pdf：`python tools/verify/verify.py reporting-compile2pdf`
  - graph_generator：`python tools/verify/verify.py reporting-graph`
  - 一键执行二者：`python tools/verify/verify.py reporting-tools`

## 测试结果读取（给 agent）

- 汇总 JSON：
  - `dist/tests/artifact/<project>/latest/test_summary.json`
  - 关键字段：`ok`、`total`、`success`、`failed`
- Python 运行日志：
  - `dist/tests/artifact/<project>/latest/test_python_output.log`
  - 包含开始时间、结束时间、return code、stdout/stderr。
- 单次运行产物（并行场景）：
  - `dist/tests/artifact/<project>/runs/<run_id>/`
- 运行时工作区：
  - `dist/tests/runtime/<project>/workspace`
  - `dist/tests/runtime/<project>/runs/<run_id>/`

## 失败排查

- 单步骤日志目录：
  - `dist/tests/artifact/<project>/latest/logs/`（latest）
  - `dist/tests/artifact/<project>/runs/<run_id>/logs/`（run 级）
- 先看 `test_summary.json` 判定失败步数，再看对应日志文件。
