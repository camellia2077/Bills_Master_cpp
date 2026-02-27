# bills_core Agent Guide

本文件是 `apps/bills_core` 的 agent 执行入口约束。详细设计不放在这里，统一在 `docs/bills_core/`。

## 1. 工作入口

- 修改前先读：`apps/bills_core/README.md`
- 详细文档索引：`docs/bills_core/module_map.md`

## 2. 变更范围约束

- 仅在需求相关目录改动，避免跨模块“顺手重构”。
- 涉及 ABI 行为变更时，必须同步检查 `docs/bills_core/abi_contract.md`。
- 不要新增重复实现，优先复用已有 ports/contracts 和 use case。

## 3. 验证要求

- 代码修改后至少执行一次：
  - `python scripts/verify.py`
- 涉及导出链路/多流水线一致性时，优先执行：
  - `python scripts/verify.py bills-parallel-smoke`
- 若失败，先修复再重复执行，直到返回码为 `0`。

## 4. 文档策略

- 这里仅放规则和导航，不放大段设计细节。
- 详细内容写入 `docs/bills_core/`，并在 `README.md`/本文件保持链接可达。
