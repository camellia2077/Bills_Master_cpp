# io Change Guide

本页用于快速判断“这次需求应该落到哪条 IO 主线”。

## 常见改动定位

| 场景 | 先看哪里 | 联动位置 |
| --- | --- | --- |
| 改配置 TOML -> 文档 DTO | `libs/io/src/io/adapters/config/` | `libs/bills_core/src/config/` |
| 改路径 -> `SourceDocumentBatch` | `libs/io/src/io/adapters/io/` | `libs/bills_core/src/common/source_document.hpp` |
| 改 JSON 文件读写 | `libs/io/src/io/adapters/io/` | `libs/bills_core/src/ingest/json/` |
| 改 sqlite 仓储或查询网关 | `libs/io/src/io/adapters/db/` | `libs/bills_core/src/ports/` |
| 改报表导出落地 | `libs/io/src/io/adapters/reports/` | `libs/bills_core/src/reporting/` |
| 改 CLI / Android 共用宿主准备 | `libs/io/src/io/host_flow_support.*` | `apps/bills_cli`、`apps/bills_android` |

## 边界提醒

- 改动涉及业务规则时，优先转到 `libs/bills_core`
- 表现层参数、导航、界面逻辑优先转到 `apps/*`

## 最小检查

1. `python tools/run.py verify import-layer-check -- --stats`
2. `python tools/run.py verify boundary-layer-check -- --stats`
