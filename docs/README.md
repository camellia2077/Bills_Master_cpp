# Docs Index

`docs/` 目录按主题组织，入口如下。

## modules

- `docs/modules/module_standards.md`
- `docs/modules/bills_core/architecture.md`
- `docs/modules/bills_core/module_map.md`
- `docs/modules/bills_core/change_guide.md`
- `docs/modules/bills_core/build_and_test.md`
- `docs/modules/bills_core/abi_contract.md`
- `docs/modules/bills_core/c_abi_boundary.md`
- `docs/modules/bills_core/c_abi_schema_draft.md`
- `docs/modules/bills_core/standard_report_json_schema_v1.md`
- `docs/modules/bills_core/legacy_pipeline_sunset_plan.md`
- `docs/modules/bills_core/md_json_render_phase0_phase1.md`
- `docs/modules/bills_io/architecture.md`
- `docs/modules/bills_io/module_map.md`
- `docs/modules/bills_io/change_guide.md`
- `docs/modules/bills_io/build_and_test.md`
- `docs/modules/bills_cli/architecture.md`
- `docs/modules/bills_cli/module_map.md`
- `docs/modules/bills_cli/change_guide.md`
- `docs/modules/bills_cli/build_and_test.md`

## release

- `docs/release/v0.3.1.md`
- `docs/release/v0.3.0.md`
- `docs/release/v0.2/v0.2.8.md`
- `docs/release/v0.2/v0.2.7.md`
- `CHANGELOG.md`（仓库根，长期维护入口）

## roadmap

- `docs/roadmap/TODO.md`（唯一待办入口）

## tools

- `docs/toolchain_sop.md`（clang-tidy / toolchain SOP 主文档）
- `docs/toolchain_tidy_automation.md`（toolchain Python 代码地图，面向 agent / 维护者）
- 统一入口（推荐）：
  - `python tools/verify/verify.py`
  - `python tools/run.py`
  - 构建/测试输出目录模型：
- `build/tests/runtime/<project>/workspace`（最新 exe/dll/config）
- `build/tests/runtime/<project>/runs/<run_id>/...`（单次运行沙箱与运行期产物）
- `build/tests/artifact/<project>/latest/...`（最新 summary/logs/exported_files 快照）
- `build/tests/artifact/<project>/runs/<run_id>/...`（单次测试归档）
- `build/tests/logic/pipeline_runner/<pipeline>/...`（流程 runner 元数据）
- `tools/toolchain/`（统一 Python toolchain 实现，含 tidy SOP 编排）
- `tests/generators/log_generator/`（测试输入数据生成器）
- 默认产物：`build/tests/artifact/log_generator/`
  - 默认配置：`tests/generators/log_generator/src/config/config.toml`
  - 显式同步夹具：`python tools/flows/log_generator_flow.py promote-fixtures`
- `tools/reporting/compile2pdf/`（报表产物转 PDF）
- `tools/reporting/graph_generator/`（报表图表生成）
  - 统一入口（独立 workflow）：
    - `python tools/verify/verify.py reporting-compile2pdf`
    - `python tools/verify/verify.py reporting-graph`
    - `python tools/verify/verify.py reporting-tools`

## 兼容脚本下线窗口（Phase 6）

- 旧入口 `tools/flows/build_then_cli_test.py`、`tools/flows/build_log_generator.py` 自 `2026-03-05` 起进入弃用窗口。
- 计划下线日期：`2026-06-30`，到期后仅保留 `tools/verify/verify.py` 与 `*flow.py` 入口。
