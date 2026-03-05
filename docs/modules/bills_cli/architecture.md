# bills_cli Architecture

`apps/bills_cli` 是组合根 + 表现层，负责命令解析、参数分发与依赖装配。

## 分层职责

- `presentation/cli/command_handler/`
  - 命令解析、命令分发与帮助信息
- `presentation/cli/controllers/`
  - 组合根控制器，连接 core use case 与 io adapters
- `main_command.cpp`
  - 程序入口与启动流程

## 约束

- 不新增核心业务规则（应落在 `libs/bills_core`）
- 不复制 IO 适配实现（应复用 `libs/bills_io`）
- 通过 `ports` 与工厂接口装配，不绕过抽象直连底层实现
