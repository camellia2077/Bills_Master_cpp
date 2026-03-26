# bills_core Module Map

本页用于快速定位 `bills_core` 当前真实主线。

## 代码目录

- `libs/bills_core/src/config/`
  - `ConfigDocumentBundle`、`ConfigBundleService`、运行时配置与校验报告
- `libs/bills_core/src/ingest/`
  - TXT 校验、转换、JSON 编解码、导入批处理
  - 重点看：
    - `ingest/bill_workflow_service.*`
    - `ingest/pipeline/`
    - `ingest/validation/`
    - `ingest/json/`
- `libs/bills_core/src/query/`
  - year/month 查询结果组装
- `libs/bills_core/src/record_template/`
  - 模板生成、记录预览、period 列举、preflight import
- `libs/bills_core/src/reporting/`
  - `StandardReport` DTO、assembler、renderer registry、格式渲染、排序
  - 重点看：
    - `reporting/standard_report/`
    - `reporting/renderers/`
    - `reporting/sorters/`
- `libs/bills_core/src/abi/`
  - 纯数据 ABI 入口：`bills_core_abi.cpp`
- `libs/bills_core/src/ports/`
  - 保留的宿主抽象：`bills_repository.hpp`、`report_data_gateway.hpp`
- `libs/bills_core/src/modules/`
  - 与 canonical 目录对齐的模块导出面与 smoke

## 改动定位建议

- 改配置文档结构化校验：先看 `config/config_bundle_service.*`
- 改 TXT 导入/转换/JSON 编解码：先看 `ingest/`
- 改 year/month 查询结果：先看 `query/query_service.*`
- 改模板生成与 preflight：先看 `record_template/`
- 改 markdown/rst/tex/typ/json 渲染：先看 `reporting/renderers/`
- 改 `StandardReport` 契约：先看 `reporting/standard_report/`
- 改 ABI 请求/响应：先看 `abi/bills_core_abi.cpp`

## 分层约束速查

- `core` 不直接访问文件系统或 sqlite
- `core` 不再暴露路径型业务接口
- 宿主若要读取配置/文档或写回导出文件，优先走 `libs/io`
