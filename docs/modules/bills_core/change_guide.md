# bills_core Change Guide

本页用于让 agent 快速判断“需求应该改哪个文件”。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 调整导入/转换主流程 | `libs/bills_core/src/billing/conversion/bills_processing_pipeline.cpp` | `libs/bills_core/src/application/use_cases/workflow_use_case.cpp` |
| 调整 TXT 解析规则 | `libs/bills_core/src/billing/conversion/modifier/processor/bills_parser.cpp` | `libs/bills_core/src/billing/conversion/modifier/processor/bills_processor.cpp` |
| 调整账单校验规则 | `libs/bills_core/src/billing/conversion/validator/bills_validator.cpp` | `libs/bills_core/src/billing/conversion/validator/config/bills_config.*` |
| 调整 JSON 序列化结构 | `libs/bills_core/src/serialization/bills_json_serializer.cpp` | `docs/modules/bills_core/standard_report_json_schema_v1.md` |
| 调整报表编排 | `libs/bills_core/src/reports/core/report_export_service.cpp` | `libs/bills_core/src/reports/standard_json/` |
| 调整 C ABI 命令行为 | `libs/bills_core/src/abi/internal/handlers/*.cpp` | `libs/bills_core/src/abi/internal/abi_shared.*`、`docs/modules/bills_core/abi_contract.md` |
| 调整模块导出面 | `libs/bills_core/src/modules/*.cppm` | `libs/bills_core/cmake/source_files.cmake` |
| 调整核心版本号 | `libs/bills_core/src/common/version.hpp` | `libs/bills_core/src/modules/common_version.cppm` |

## 变更边界提醒

- 业务规则优先改 `libs/bills_core/src`，不要放到 `apps/bills_cli`。
- 平台依赖（sqlite、文件系统插件装配）优先放 `libs/bills_io`。
- ABI 行为改动必须同步核对 `docs/modules/bills_core/abi_contract.md`。

## 提交前最小检查

1. 跑 `python tools/verify/verify.py import-layer-check --stats`
2. 跑 `python tools/verify/verify.py boundary-layer-check --stats`
3. 跑 `python tools/verify/verify.py`
