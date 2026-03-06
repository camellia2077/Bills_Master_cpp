# bills_io Change Guide

本页用于帮助快速定位 `libs/bills_io` 的改动入口。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 修改文本账单读取行为 | `libs/bills_io/src/bills_io/adapters/io/file_bill_content_reader.cpp` | `file_bill_content_reader.hpp` |
| 修改输入目录遍历策略 | `libs/bills_io/src/bills_io/adapters/io/file_bill_file_enumerator.cpp` | `file_bill_file_enumerator.hpp` |
| 修改 TOML 配置解析 | `libs/bills_io/src/bills_io/adapters/config/toml_config_provider.cpp` | `toml_bill_config_loader.*`、`toml_modifier_config_loader.*` |
| 修改账单入库路径 | `libs/bills_io/src/bills_io/adapters/db/sqlite_bill_repository.cpp` | `bill_inserter.*`、`database_manager.*` |
| 修改查询数据聚合 | `libs/bills_io/src/bills_io/adapters/db/sqlite_report_data_gateway.cpp` | `month_query.*`、`year_query.*` |
| 修改内建 formatter 装配 | `libs/bills_io/src/bills_io/adapters/reports/builtin_month_report_formatter_provider.cpp` | `builtin_yearly_report_formatter_provider.*`、`io_factory.*` |
| 修改默认注入实现 | `libs/bills_io/src/bills_io/io_factory.cpp` | `libs/bills_io/src/bills_io/io_factory.hpp` |

## 变更边界提醒

- 若改动涉及业务规则，请转到 `libs/bills_core`
- 若改动涉及 CLI 参数解析，请转到 `apps/bills_cli`

## 提交前最小检查

1. `python tools/verify/verify.py`
