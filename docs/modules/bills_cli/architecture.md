# bills_cli Architecture

`apps/bills_cli` 现在采用“命令组 + 子命令”模型，是 `libs/core` / `libs/io` 之上的组合根与表现层。

## 事实源目录

- `apps/bills_cli/src/presentation/entry/`
  - CLI 启动入口、runtime context、路由总装配
- `apps/bills_cli/src/presentation/parsing/`
  - CLI11 命令树、分层 help 与 argv -> typed request 的解析
- `apps/bills_cli/src/presentation/features/workspace/`
  - `workspace validate/convert/ingest/import-json`
- `apps/bills_cli/src/presentation/features/report/`
  - `report show/export`
- `apps/bills_cli/src/presentation/features/template/`
  - `template generate/preview/list-periods`
- `apps/bills_cli/src/presentation/features/config/`
  - `config inspect/formats`
- `apps/bills_cli/src/presentation/features/meta/`
  - `meta version/notices`

## 职责边界

- parser 只负责命令树、help 与 typed request，不承担业务编排。
- feature handler 只负责一类任务域，不跨域拼装其他命令。
- runtime context 只负责宿主路径、默认目录与格式启用信息。
- 业务规则继续留在 `libs/core`。
- 文件读写、配置读取、数据库/导出适配继续落在 `libs/io`。

## 约束

- 不恢复旧的旗标式入口，也不保留兼容壳。
- 不在 CLI 再造“大而全 controller”；公共宿主逻辑上限仍是 `libs/io/host_flow_support.*`。
- 改 CLI 行为时，优先从对应 feature handler 入手，而不是跨 feature 混改。
