# bills_core Architecture

## 目标

在保持 `apps/bills_core/src` 功能稳定前提下，保持“核心业务逻辑可测试、ABI 边界清晰、平台适配可替换”。

## 分层建议

- `domain/`：领域对象与规则（若暂未承载实质模型，可保持最小化，避免假分层）。
- `use_cases/`：纯业务流程编排，不依赖 UI/CLI 输出。
- `ports/contracts/`：跨模块共享 DTO 与接口契约。
- `adapters/`：数据库、文件系统、格式化器、ABI/CLI 适配实现。

## 约束

- 核心层不要直接 `std::cout/std::cerr`。
- ABI 层只做参数解包、结果封装、错误码映射。
- 新功能优先在 use case 扩展，再通过 adapter 接入。

## 相关阅读

- `docs/bills_core/module_map.md`
- `docs/bills_core/abi_contract.md`

