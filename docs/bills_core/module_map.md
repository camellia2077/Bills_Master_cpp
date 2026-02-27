# bills_core Module Map

本页用于让 agent 快速定位“改哪里”。

## 代码目录

- `apps/bills_core/src/billing/`
  - 账单转换、导入、查询、导出相关主流程。
- `apps/bills_core/src/reports/`（若仍存在）
  - 报表相关实现；共享 DTO/接口建议迁移到 `ports/contracts` 后再清理旧目录。
- `apps/bills_core/src/*abi*`
  - C ABI 入口与命令处理。

## 改动定位建议

- 改业务规则：优先找 use case / processor / service。
- 改跨模块数据结构：优先找 `ports/contracts`。
- 改 C 接口行为：优先找 ABI handler 与 schema 映射。
- 改构建行为：看 `apps/bills_core/CMakeLists.txt` 和 `apps/bills_core/cmake/`。

## 执行顺序建议

1. 先在本文件定位目录。
2. 再看对应专题文档（architecture / abi / build_and_test）。
3. 最后改代码并跑 `python scripts/verify.py`。

