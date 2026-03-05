# C++ Modules 仓库统一规范

本规范用于 Phase 5 仓库级推广，约束新增或迁移模块的命名、可见性与依赖方向。

## 1. 模块命名规则

- 格式：`bill.<layer>.<feature>`
- `layer` 取值建议：
  - `core`：核心业务域与用例。
  - `io`：文件、数据库、外部系统适配。
  - `cli`：命令行表现层。
  - `tools`：工具链或脚本辅助模块。
- `feature` 要求：
  - 使用小写英文与下划线（示例：`report_export`、`config_validator`）。
  - 对应一个清晰职责，不混合多个领域。

示例：

- `bill.core.billing`
- `bill.core.reports`
- `bill.io.sqlite_gateway`
- `bill.cli.command_handler`

## 2. 模块可见性规则

- 对外 API：
  - 通过接口单元 `export module ...;` 暴露稳定契约。
  - 仅导出必要类型与函数，禁止导出内部工具函数。
- 模块内部细节：
  - 使用分区单元 `module <name>:<partition>;` 承载内部实现。
  - 分区命名建议：`internal_*` 或按子职责命名，如 `parsing`、`querying`。
- 兼容期策略：
  - 允许保留头文件 facade，但 facade 仅转发，不新增业务逻辑。
  - 新逻辑优先落在模块单元，避免双实现漂移。

## 3. 依赖方向规则

- 强制依赖方向：
  - `bill.cli.*` -> `bill.io.*` / `bill.core.*`
  - `bill.io.*` -> `bill.core.*`（仅通过 ports/contract）
  - `bill.core.*` -> 不得依赖 `bill.io.*` 或 `bill.cli.*`
- 禁止逆向依赖：
  - `core` 不允许 import/include `apps/...` 或平台适配实现。
  - `io` 不允许依赖 `cli` 表现层控制器、命令处理器。
- 端口优先：
  - 跨层交互必须通过 `ports/*`、`contracts/*` 抽象接口。
  - 具体技术实现（sqlite、filesystem、plugin loader）仅放在 `io/cli` 层。

## 4. 验证要求

- 每次涉及模块化改动，至少执行：
  - `python tools/verify/verify.py module-mode-check`
- 每次涉及工具链脚本改动，至少执行：
  - `python tools/verify/verify.py tools-layer-check`
- 全量验证建议执行：
  - `python tools/verify/verify.py all-tests`

## 5. tools/* 分层规则

- 目标：
  - 工具链脚本与业务代码解耦，通过 CLI/子进程边界交互，而非 Python import 直连。
- 规则：
  - `tools/build/*` 不得 import `tools.verify.*`。
  - `tools/verify/*` 不得 import `tools.build.*`（应调用脚本入口）。
  - `tools/*` 不得直接 import `apps.*`、`libs.*`、`tests.*`。
- 门禁：
  - `python tools/verify/verify.py tools-layer-check`
