# bills_core

`apps/bills_core` 是账单核心业务模块，负责数据转换、导入、查询、导出与 ABI 适配。

## 快速导航

- Agent 规则：`apps/bills_core/AGENTS.md`
- 架构说明：`docs/bills_core/architecture.md`
- 目录地图：`docs/bills_core/module_map.md`
- 构建与测试：`docs/bills_core/build_and_test.md`
- ABI 约束：`docs/bills_core/abi_contract.md`

## 目录概览

- `src/`：核心源码
- `cmake/`：构建配置片段
- `CMakeLists.txt`：模块构建入口

## 本地验证

- 推荐统一入口：
  - `python scripts/verify.py`

## 维护原则

- KISS: 优先最小改动，先修正确性再做重构。
- DRY: 避免重复 DTO、重复解析流程、重复命令处理。
- YAGNI: 不为“可能将来需要”提前引入复杂抽象。

