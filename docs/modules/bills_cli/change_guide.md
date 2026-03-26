# bills_cli Change Guide

本页用于让 agent 快速判断 `apps/bills_cli` 要改哪些文件。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 新增/调整命令协议 | `apps/bills_cli/src/presentation/parsing/cli_parser.cpp` | `cli_request.hpp` |
| 调整 `workspace` 行为 | `apps/bills_cli/src/presentation/features/workspace/workspace_handler.cpp` | `presentation/entry/runtime_context.cpp` |
| 调整 `report show/export` 行为 | `apps/bills_cli/src/presentation/features/report/report_handler.cpp` | `presentation/entry/runtime_context.cpp` |
| 调整模板相关行为 | `apps/bills_cli/src/presentation/features/template/template_handler.cpp` | `libs/io/src/io/host_flow_support.*` |
| 调整 config/meta 命令 | `apps/bills_cli/src/presentation/features/config/config_handler.cpp` / `apps/bills_cli/src/presentation/features/meta/meta_handler.cpp` | `apps/bills_cli/src/common/cli_version.hpp` |
| 调整帮助文本 / help 分层 | `apps/bills_cli/src/presentation/parsing/cli_parser.cpp` | `cli_request.hpp` |

## 变更边界提醒

- parser 只做路由和参数转换，不写核心业务规则。
- feature handler 只做一个任务域的表现层编排。
- 核心流程变更请转到 `docs/modules/bills_core/change_guide.md`。
- IO 行为变更请转到 `docs/modules/io/change_guide.md`。

## 提交前最小检查

1. 跑 `python tools/run.py verify bills-tracer`
2. 跑 `python tools/run.py verify`
