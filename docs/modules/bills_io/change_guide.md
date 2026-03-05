# bills_io Change Guide

本页用于让 agent 快速判断 `libs/bills_io` 要改哪些文件。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 修改文本文件读取行为 | `libs/bills_io/src/bills_io/adapters/io/file_bill_content_reader.cpp` | `file_bill_content_reader.hpp` |
| 修改输入目录遍历策略 | `libs/bills_io/src/bills_io/adapters/io/file_bill_file_enumerator.cpp` | `file_bill_file_enumerator.hpp` |
| 修改配置 JSON 解析 | `libs/bills_io/src/bills_io/adapters/config/json_config_provider.cpp` | `json_bill_config_loader.*`、`json_modifier_config_loader.*` |
| 修改账单入库路径 | `libs/bills_io/src/bills_io/adapters/db/sqlite_bill_repository.cpp` | `bill_inserter.*`、`database_manager.*` |
| 修改查询数据聚合 | `libs/bills_io/src/bills_io/adapters/db/sqlite_report_data_gateway.cpp` | `month_query.*`、`year_query.*` |
| 修改插件动态加载 | `libs/bills_io/src/bills_io/adapters/plugins/common/plugin_loader.hpp` | `dynamic_*_report_formatter_provider.*` |
| 修改注入装配默认实现 | `libs/bills_io/src/bills_io/io_factory.cpp` | `libs/bills_io/src/bills_io/io_factory.hpp` |

## 变更边界提醒

- 若改动涉及业务规则，请转到 `libs/bills_core` 对应 use case/service。
- 若改动涉及 CLI 参数解析，请转到 `apps/bills_cli`。

## 提交前最小检查

1. 跑 `python tools/verify/verify.py`
