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

- 统一入口（推荐）：
  - `python tools/verify/verify.py`
  - 构建/测试输出目录模型：
    - `tests/output/runtime/<project>/workspace`（运行时 exe/dll/config）
    - `tests/output/artifact/<project>/runs/<run_id>/...`（日志与导出产物）
    - `tests/output/logic/<project>/...`（逻辑测试与 runner 元数据）
- `tests/generators/log_generator/`（测试输入数据生成器）
  - 默认产物：`tests/output/artifact/log_generator/`
  - 显式同步夹具：`python tools/build/log_generator_flow.py promote-fixtures`
- `tools/reporting/compile2pdf/`（报表产物转 PDF）
- `tools/reporting/graph_generator/`（报表图表生成）
  - 统一入口（独立 workflow）：
    - `python tools/verify/verify.py reporting-compile2pdf`
    - `python tools/verify/verify.py reporting-graph`
    - `python tools/verify/verify.py reporting-tools`

## 兼容脚本下线窗口（Phase 6）

- 旧入口 `tools/build/build_then_cli_test.py`、`tools/build/build_log_generator.py` 自 `2026-03-05` 起进入弃用窗口。
- 计划下线日期：`2026-06-30`（到期后仅保留 `tools/verify/verify.py` 与 `*flow.py` 入口）。
