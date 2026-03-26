# core Change Guide

本页用于快速判断“这次需求应该落到哪条主线”。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 调整配置文档校验或运行时配置装配 | `libs/core/src/config/config_bundle_service.cpp` | `libs/core/src/config/config_document_types.hpp` |
| 调整 TXT 校验、转换、导入批处理 | `libs/core/src/ingest/bill_workflow_service.cpp` | `libs/core/src/ingest/pipeline/`、`libs/core/src/ingest/validation/` |
| 调整账单 JSON 编解码 | `libs/core/src/ingest/json/bills_json_serializer.cpp` | `libs/io/src/io/adapters/io/json_bill_document_io.cpp` |
| 调整 year/month 查询逻辑 | `libs/core/src/query/query_service.cpp` | `libs/core/src/ports/report_data_gateway.hpp` |
| 调整 `StandardReport` 结构或组装 | `libs/core/src/reporting/standard_report/standard_report_assembler.cpp` | `libs/core/src/reporting/standard_report/standard_report_dto.hpp` |
| 调整 markdown/rst/tex/typ/json 渲染 | `libs/core/src/reporting/renderers/` | `libs/core/src/reporting/renderers/standard_report_renderer_registry.*` |
| 调整模板生成、记录预览、preflight import | `libs/core/src/record_template/record_template_service.cpp` | `libs/core/src/record_template/import_preflight_service.cpp` |
| 调整 ABI 请求/响应结构 | `libs/core/src/abi/bills_core_abi.cpp` | `docs/modules/core/abi_contract.md` |
| 调整模块导出面 | `libs/core/src/modules/*.cppm` | `libs/core/cmake/source_files.cmake` |

## 边界提醒

- 需要读 TOML / 读文件 / 写文件 / 枚举目录时，不要回到 `core`，优先改 `libs/io`
- 需要改 CLI / Android 共用宿主拼装时，优先看 `libs/io/src/io/host_flow_support.*`
- 需要新增业务能力时，先落 `core` 的纯数据服务，再让宿主去接

## 提交前最小检查

1. `python tools/run.py dist bills-tracer-cli --preset debug --scope shared`
2. `python tools/run.py dist bills-tracer-android --preset debug`
3. `python tools/run.py verify import-layer-check -- --stats`
4. `python tools/run.py verify report-consistency-gate`
5. `python tools/run.py dist bills-tracer-core --preset debug`
6. `python tools/run.py verify module-mode-check`
