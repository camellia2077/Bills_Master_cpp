# bills_core Module Map

本页用于让 agent 快速定位“改哪里”。

## 代码目录

- `libs/bills_core/src/billing/`
  - 账单转换、导入、查询、导出相关主流程。
- `libs/bills_core/src/reports/`
  - 报表编排与标准 JSON 渲染实现。
- `libs/bills_core/src/*abi*`
  - C ABI 入口与命令处理。
- `libs/bills_core/src/ports/`
  - 核心端口抽象（仓储、序列化、数据网关、格式 provider 等）。
- `products/bills_cli/src/windows/presentation/`
  - CLI 命令分发与控制器（非 core）。
- `products/bills_cli/src/windows/infrastructure/`
  - 文件系统、sqlite、插件加载等适配实现（非 core）。

## 改动定位建议

- 改业务规则：优先找 use case / processor / service。
- 改跨模块接口契约：优先找 `ports/`。
- 改 C 接口行为：优先找 ABI handler 与 schema 映射。
- 改构建行为：看 `libs/bills_core/CMakeLists.txt` 和 `libs/bills_core/cmake/`。

## 分层约束速查

- core 不得依赖 platform/windows 实现。
- cli 不得新增业务规则实现（应落到 core use case / reports / ports）。

## 执行顺序建议

1. 先在本文件定位目录。
2. 再看对应专题文档（architecture / abi / build_and_test）。
3. 最后改代码并跑 `python tools/verify/verify.py`。
