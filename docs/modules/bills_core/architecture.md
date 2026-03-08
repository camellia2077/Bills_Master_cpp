# bills_core Architecture

## 目标

在保持 `libs/bills_core/src` 功能稳定前提下，保持“核心业务逻辑可测试、ABI 边界清晰、平台适配可替换”。
当前分层收口目标：
- `libs/bills_core` 仅承载平台无关业务能力（use case、ports、核心报表编排与渲染）。
- `apps/bills_cli` 仅承载 presentation + infrastructure adapters（Windows/SQLite/插件装配与运行时 IO）。

## 分层建议

- `application/use_cases/`：纯业务流程编排，不依赖 UI/CLI 输出。
- `ports/`：核心对外依赖抽象（仓储、文件枚举、序列化、报表网关、格式提供器等）。
- `reports/`：平台无关报表编排与标准 JSON 渲染逻辑（markdown/latex/json）。
- `abi/`：C ABI 边界层，仅做命令解包、调用、结果封装。

## 约束

- core 不得依赖平台实现：
  - 禁止 `windows/` 路径 include；
  - 禁止直接依赖平台 API（如 `LoadLibrary`）；
  - 禁止在 core 直接耦合 sqlite 平台细节。
- 核心层不要直接 `std::cout/std::cerr`。
- ABI 层只做参数解包、结果封装、错误码映射。
- 新功能优先在 use case 扩展，再通过 adapter 接入。

## 相关阅读

- `docs/modules/bills_core/module_map.md`
- `docs/modules/bills_core/change_guide.md`
- `docs/modules/bills_core/abi_contract.md`
- `docs/modules/bills_core/dist_and_test.md`
