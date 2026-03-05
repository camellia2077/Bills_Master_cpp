# bills_cli Module Map

本页用于快速定位 `apps/bills_cli` 的改动入口。

## 代码目录

- `apps/bills_cli/src/windows/presentation/cli/main_command.cpp`
  - CLI 程序入口
- `apps/bills_cli/src/windows/presentation/cli/command_handler/`
  - 命令路由、命令参数处理、帮助输出
- `apps/bills_cli/src/windows/presentation/cli/controllers/`
  - 业务调用编排、导出/工作流组合根控制
- `apps/bills_cli/src/common/cli_version.hpp`
  - CLI 表现层版本号

## 改动定位建议

- 改命令参数协议：先看 `command_handler/command_dispatcher.cpp`
- 改命令执行行为：先看 `command_handler/commands/*.cpp`
- 改流程装配：先看 `controllers/workflow/workflow_controller.cpp`
- 改导出装配：先看 `controllers/export/export_controller.cpp`
- 改版本显示：先看 `controllers/app_controller.cpp` + `src/common/cli_version.hpp`
