# bills_core Dist And Test

改动定位建议先看：`docs/modules/bills_core/change_guide.md`

## 统一入口

- `python tools/run.py`
- `python tools/run.py dist <target>`

默认验收通过 `python tools/run.py verify ...` 触发；`dist` 入口负责把 CMake 产物统一落到 `dist/cmake/`。

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
  - `python tools/run.py verify logic-tests`
  - 默认包含：`python tools/run.py dist bills-tracer-core --preset debug`
- 结果产物测试（integration/e2e/snapshot）：
  - `python tools/run.py verify artifact-tests`
  - 默认包含：`bills-tracer` + `report-consistency-gate`
- 全量测试（内部 + 产物）：
  - `python tools/run.py verify all-tests`
  - 默认为开发模式：不阻塞性能门禁，仅做一致性校验。
  - 包含模块模式双通道检查：`BILLS_ENABLE_MODULES=OFF/ON`。
  - 若需要阻塞性能门禁：
    - `python tools/run.py verify artifact-tests -- --scope full --enforce-performance-gate`

## Python Unittest 运行方式

- 对 `tests/suites/toolchain/`、`tests/suites/reporting/` 下的 Python unittest，推荐从仓库根目录使用模块方式运行：
  - `python -m unittest tests.suites.toolchain.test_verify_cli`
  - `python -m unittest tests.suites.toolchain.test_import_layering`
  - `python -m unittest tests.suites.toolchain.test_boundary_layering`
  - `python -m unittest tests.suites.toolchain.test_pipeline_runner`
- 不推荐直接执行 `python tests/suites/.../*.py`，这类脚本的 import 解析依赖仓库根目录模块上下文。

## 目录分组（logic / artifact）

- 测试脚本目录：
  - 产物层：`tests/suites/artifact/bills_tracer/run_tests.py`
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
  - `python tools/run.py verify bills-tracer-parallel-smoke`
  - 可选：
    - `python tools/run.py verify bills-tracer-parallel-smoke -- --compare-scope all`
    - `python tools/run.py verify bills-tracer-parallel-smoke -- --model-project bills_a --json-project bills_b`
- 仅准备 bills CLI：
  - `python tools/run.py dist bills-tracer-cli --preset debug --scope shared`
- 仅准备 `bills_core`：
  - `python tools/run.py dist bills-tracer-core --preset debug`
- 仅准备 log_generator：
  - `python tools/run.py log-generator dist --preset debug`
  - 默认 TOML pipeline：`tools/verify/pipelines/log_generator_dist.toml`
- log_generator 完整命令行测试：
  - `python tools/run.py verify bills-tracer-log-generator-cli-test`
  - 默认 TOML pipeline：`tools/verify/pipelines/log_generator_cli.toml`
- log_generator 运行时配置默认位于：
  - `dist/cmake/log_generator/debug/shared/bin/config/config.toml`
- 生成 log_generator 测试输入（默认落盘到 artifact）：
  - `python tools/run.py log-generator generate --preset debug --start-year 2024 --end-year 2024`
  - 生成命令会将 `config.toml` 复制到独立 runtime 目录后再执行生成器。
- 显式将 log_generator 数据提升到共享 testdata：
  - `python tools/run.py log-generator promote-testdata`
  - 执行记录：`dist/tests/artifact/log_generator/last_promote.json`
- 模块模式双通道检查：
  - `python tools/run.py verify module-mode-check`
  - 可选参数示例：
    - `python tools/run.py verify module-mode-check -- --preset release`
- Windows 原生静态构建前提：
  - 必须提供 `C:/msys64/mingw64/bin` 工具链环境
  - Windows 下不再保留 `bills_core` shared-library / 专用 ABI smoke 路径
  - `python tools/run.py import-gate bills-tracer-cli --preset debug --scope shared`
  - `python tools/run.py import-gate bills-tracer-log-generator --preset debug --scope shared`
- tools 分层约束检查：
  - `python tools/run.py verify tools-layer-check`
- 一致性门禁（含性能阈值）：
  - `python tools/run.py verify report-consistency-gate`
  - 默认格式：`md,json,tex`
  - 默认性能阈值：`model-first` 相比 `json-first` 不超过 `+10%`
- TOML 流程 Runner：
  - 通用入口：`python tools/run.py verify pipeline-run -- --config tools/verify/pipelines/bills_artifact.toml`
  - bills_tracer 示例：`python tools/run.py verify pipeline-bills-tracer`
  - log_generator 示例：`python tools/run.py verify pipeline-bills-tracer-log-generator`
- reporting workflow（独立于核心门禁）：
  - compile2pdf：`python tools/run.py verify reporting-compile2pdf`
  - graph_generator：`python tools/run.py verify reporting-graph`
  - 一键执行二者：`python tools/run.py verify reporting-tools`

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
