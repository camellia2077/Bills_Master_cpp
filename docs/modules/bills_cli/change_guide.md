# bills_cli Change Guide

本页用于让 agent 快速判断 `apps/bills_cli` 要改哪些文件。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 新增/调整命令入口 | `apps/bills_cli/src/windows/presentation/cli/command_handler/command_dispatcher.cpp` | `command_handler/commands/*.hpp` |
| 修改 `ingest/query/export` 命令参数 | `apps/bills_cli/src/windows/presentation/cli/command_handler/commands/*.cpp` | `controllers/app_controller.cpp` |
| 调整工作流装配（validate/convert/import） | `apps/bills_cli/src/windows/presentation/cli/controllers/workflow/workflow_controller.cpp` | `workflow/path_builder.*` |
| 调整导出装配（format/pipeline） | `apps/bills_cli/src/windows/presentation/cli/controllers/export/export_controller.cpp` | `controllers/app_controller.cpp` |
| 调整 CLI 版本输出 | `apps/bills_cli/src/windows/presentation/cli/controllers/app_controller.cpp` | `apps/bills_cli/src/common/cli_version.hpp` |
| 调整帮助文本 | `apps/bills_cli/src/windows/presentation/cli/command_handler/usage_help.cpp` | `usage_help.hpp` |

## 变更边界提醒

- 命令层只做路由和参数转换，不写核心业务规则。
- 核心流程变更请转到 `docs/modules/bills_core/change_guide.md`。
- IO 行为变更请转到 `docs/modules/bills_io/change_guide.md`。

## 提交前最小检查

1. 跑 `python tools/verify/verify.py bills`
2. 跑 `python tools/verify/verify.py`
