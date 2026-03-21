# bills_cli Module Map

本页用于快速定位 `apps/bills_cli` 的改动入口。

## 代码目录

- `apps/bills_cli/src/presentation/entry/main_command.cpp`
  - CLI 程序入口
- `apps/bills_cli/src/presentation/entry/cli_app.cpp`
  - request 路由与 feature dispatch
- `apps/bills_cli/src/presentation/entry/runtime_context.cpp`
  - 运行时目录、默认 DB、导出目录、notices/format helper
- `apps/bills_cli/src/presentation/parsing/cli_parser.cpp`
  - 新命令组语法解析
- `apps/bills_cli/src/presentation/features/workspace/workspace_handler.cpp`
  - 工作区导入链路
- `apps/bills_cli/src/presentation/features/report/report_handler.cpp`
  - 查询展示与导出链路
- `apps/bills_cli/src/presentation/features/template/template_handler.cpp`
  - 模板生成、预览、period 列举
- `apps/bills_cli/src/presentation/features/config/config_handler.cpp`
  - 配置检查与格式列表
- `apps/bills_cli/src/presentation/features/meta/meta_handler.cpp`
  - 版本与 notices
- `apps/bills_cli/src/presentation/output/help_text.cpp`
  - 帮助文本
- `apps/bills_cli/src/common/cli_version.hpp`
  - CLI 表现层版本号

## 改动定位建议

- 改顶层命令协议：先看 `presentation/parsing/cli_parser.cpp`
- 改 `workspace` 行为：先看 `presentation/features/workspace/workspace_handler.cpp`
- 改 `report` 行为：先看 `presentation/features/report/report_handler.cpp`
- 改 `template` 行为：先看 `presentation/features/template/template_handler.cpp`
- 改配置/元信息输出：先看 `presentation/features/config/` 或 `presentation/features/meta/`
- 改路径、默认 DB、notices、启用格式：先看 `presentation/entry/runtime_context.cpp`
