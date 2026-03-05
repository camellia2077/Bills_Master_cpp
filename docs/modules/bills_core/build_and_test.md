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
  - 运行时层：`tests/output/runtime/<project>/`
    - `workspace/`（exe/dll/config）
    - `runs/<run_id>/`（每次执行独立运行目录）
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
  - Phase 4 起默认由 TOML runner 驱动（`log-build` 无参数时直接使用 `tools/verify/pipelines/log_generator_build.toml`）。
- log_generator 完整命令行测试（构建 + CLI 参数覆盖）：
  - `python tools/verify/verify.py log-cli-test`
  - 默认由 TOML runner 执行 `tools/verify/pipelines/log_generator_cli.toml`。
- 生成 log_generator 测试输入（默认落盘到 artifact）：
  - `python tools/build/log_generator_flow.py generate --mode Debug --start-year 2024 --end-year 2024`
- 显式将 log_generator 数据提升到 fixtures（默认不会覆盖夹具）：
  - `python tools/build/log_generator_flow.py promote-fixtures`
- 模块模式双通道检查：
  - `python tools/verify/verify.py module-mode-check`
  - 可选参数示例：
    - `python tools/verify/verify.py module-mode-check -- --command build`
    - `python tools/verify/verify.py module-mode-check -- --compiler clang`
- tools 分层约束检查：
  - `python tools/verify/verify.py tools-layer-check`
- 一致性门禁（含性能阈值）：
  - `python tools/verify/verify.py report-consistency-gate`
  - 默认格式：`md,json,tex`
  - 默认性能阈值：`model-first` 相比 `json-first` 不超过 `+10%`
- TOML 流程 Runner（Phase 2）：
  - 通用入口：`python tools/verify/verify.py pipeline-run -- --config tools/verify/pipelines/bills_artifact.toml`
  - bills 示例：`python tools/verify/verify.py pipeline-bills`
  - log_generator 示例：`python tools/verify/verify.py pipeline-log-generator`
- Phase 3 收敛说明：
  - `python tools/verify/verify.py bills` 已切到 TOML runner 驱动（命令兼容保持不变）。
  - `python tools/verify/verify.py bills-parallel-smoke` / `report-consistency-gate` 也由 TOML pipeline 编排。
- Phase 4 收敛说明：
  - `python tools/build/build_log_generator.py` 为兼容壳层，已改为直接转发 `tools/build/log_generator_flow.py`。
  - `promote-fixtures` 为显式步骤，执行记录落在 `tests/output/artifact/log_generator/last_promote.json`。
- Phase 6 兼容入口下线窗口：
  - `tools/build/build_then_cli_test.py`、`tools/build/build_log_generator.py` 自 `2026-03-05` 起输出弃用提示。
  - 计划下线日期：`2026-06-30`。
  - 推荐入口：`python tools/verify/verify.py`（工作流）或 `tools/build/*_flow.py`（底层流程）。
- Phase 5（reporting workflow，独立于核心门禁）：
  - compile2pdf：`python tools/verify/verify.py reporting-compile2pdf`
  - graph_generator：`python tools/verify/verify.py reporting-graph`
  - 一键执行二者：`python tools/verify/verify.py reporting-tools`
  - 这些 workflow 默认不并入 `bills` / `report-consistency-gate`。

## 测试结果读取（给 agent）

- 汇总 JSON：
  - `tests/output/artifact/<project>/test_summary.json`
  - 关键字段：`ok`、`total`、`success`、`failed`
- Python 运行日志：
  - `tests/output/artifact/<project>/test_python_output.log`
  - 包含开始时间、结束时间、return code、stdout/stderr。
- 单次运行产物（并行场景）：
  - `tests/output/artifact/<project>/runs/<run_id>/`
- 运行时工作区（兼容旧路径镜像）：
  - `tests/output/runtime/<project>/workspace`

## 失败排查

- 单步骤日志目录：
  - `tests/output/artifact/<project>/logs/`（latest）
  - `tests/output/artifact/<project>/runs/<run_id>/logs/`（run 级）
- 先看 `test_summary.json` 判定失败步数，再看对应日志文件。
