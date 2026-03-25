# Docs Index

`docs/` 目录按主题组织，入口如下。

## modules

- `docs/modules/module_standards.md`
- `docs/modules/bills_core/README.md`
- `docs/modules/bills_core/architecture.md`
- `docs/modules/bills_core/module_map.md`
- `docs/modules/bills_core/change_guide.md`
- `docs/modules/bills_core/dist_and_test.md`
- `docs/modules/bills_core/abi_contract.md`
- `docs/modules/bills_core/c_abi_boundary.md`
- `docs/modules/bills_core/c_abi_schema_draft.md`
- `docs/modules/bills_core/standard_report_json_schema_v1.md`
- `docs/modules/bills_core/legacy_pipeline_sunset_plan.md`
- `docs/modules/bills_core/md_json_render_phase0_phase1.md`
- `docs/modules/bills_io/README.md`
- `docs/modules/bills_io/architecture.md`
- `docs/modules/bills_io/module_map.md`
- `docs/modules/bills_io/change_guide.md`
- `docs/modules/bills_io/dist_and_test.md`
- `docs/modules/bills_android/README.md`
- `docs/modules/bills_android/architecture.md`
- `docs/modules/bills_android/module_map.md`
- `docs/modules/bills_android/change_guide.md`
- `docs/modules/bills_android/dist_and_test.md`
- `docs/modules/bills_cli/architecture.md`
- `docs/modules/bills_cli/module_map.md`
- `docs/modules/bills_cli/change_guide.md`
- `docs/modules/bills_cli/dist_and_test.md`

## release

- `docs/release/README.md`（统一发布历史入口）
- `docs/release/product/`（跨模块 / 跨端的产品级历史）
- `docs/release/core/`（`bills_core` 内核能力历史）
- `docs/release/clients/cli/`（`bills_cli` 表现层历史）
- `docs/release/clients/android/`（`bills_android` 表现层历史）

## roadmap

- `docs/roadmap/TODO.md`（唯一待办入口）

## notes

- `docs/notes/android_apk_build.md`（Android APK 编译命令与产物说明）
- `docs/notes/windows_exe_build.md`（Windows bills_tracer_cli 编译命令与产物说明）

## tools

- `docs/toolchain_sop.md`（clang-tidy / toolchain SOP 主文档）
- `docs/toolchain_tidy_automation.md`（toolchain Python 代码地图，面向 agent / 维护者）
- 统一入口（推荐）：
  - `python tools/verify/verify.py`
  - `python tools/run.py`
  - `python tools/run.py dist <target>`
  - dist 目录模型：
- `dist/cmake/<target>/<preset>/<scope>/...`（CMake 产物）
- `dist/runtime/<project>/workspace/...`（手工 CLI 默认运行时：db/cache/exports/config/notices）
- `dist/tests/runtime/<project>/workspace`（最新 exe/dll/config）
- `dist/tests/runtime/<project>/runs/<run_id>/...`（单次运行沙箱与运行期产物）
- `dist/tests/artifact/<project>/latest/...`（最新 summary/logs/exports 快照）
- `dist/tests/artifact/<project>/runs/<run_id>/...`（单次测试归档）
- `dist/tests/logic/pipeline_runner/<pipeline>/...`（流程 runner 元数据）
- Android / Gradle 默认产物位于各模块自己的 `build/` 目录；Android native CMake staging 默认位于模块下 `.cxx/`
- `tools/toolchain/`（统一 Python toolchain 实现，含 tidy SOP 编排）
- `tests/generators/log_generator/`（测试输入数据生成器）
- `testdata/bills/`（跨端共享 canonical 输入数据）
- 默认产物：`dist/tests/artifact/log_generator/`
  - 默认配置：`tests/generators/log_generator/scripts/config.toml`
  - 显式同步数据：`python tools/flows/bills_tracer_log_generator_flow.py promote-testdata`
- `tools/reporting/compile2pdf/`（报表产物转 PDF）
- `tools/reporting/graph_generator/`（报表图表生成）
  - 统一入口（独立 workflow）：
    - `python tools/verify/verify.py reporting-compile2pdf`
    - `python tools/verify/verify.py reporting-graph`
    - `python tools/verify/verify.py reporting-tools`
