# bills_core

`libs/bills_core` 是账单核心业务模块，负责数据转换、导入、查询、导出与 ABI 适配。

## 快速导航

- Agent 规则：`libs/bills_core/AGENTS.md`
- 架构说明：`docs/modules/bills_core/architecture.md`
- 目录地图：`docs/modules/bills_core/module_map.md`
- 构建与测试：`docs/modules/bills_core/build_and_test.md`
- ABI 约束：`docs/modules/bills_core/abi_contract.md`

## 目录概览

- `src/`：核心源码
- `cmake/`：构建配置片段
- `CMakeLists.txt`：模块构建入口

## 本地验证

- 推荐统一入口：
  - `python tools/verify/verify.py`
- 内部逻辑测试（unit/component）：
  - `python tools/verify/verify.py logic-tests`
- 结果产物测试（integration/e2e/snapshot）：
  - `python tools/verify/verify.py artifact-tests`
- 全量测试（内部 + 产物）：
  - `python tools/verify/verify.py all-tests`
- 并行一致性冒烟：
  - `python tools/verify/verify.py bills-parallel-smoke`
- 一致性 + 性能门禁：
  - `python tools/verify/verify.py report-consistency-gate`

## 测试目录约定

- logic（内部逻辑）：
  - 脚本：`tests/suites/logic/bills_core_abi/run_tests.py`
  - 输出：`tests/output/logic/`
- artifact（结果产物）：
  - 脚本：`tests/suites/artifact/bills_master/run_tests.py`
  - 输出：`tests/output/artifact/<project>/`

## 导出流水线状态

- 当前默认：`model-first`
- 保留回退：`legacy`
- 保留对照：`json-first`

## legacy 下线前置条件

- 连续通过一致性门禁（`report-consistency-gate`）且无数据差异。
- `model-first` 与 `json-first` 在 `md/json/tex/typ` 对比范围稳定一致。
- 回归窗口内无阻塞级线上问题需依赖 `legacy` 回退。
- 已发布迁移说明，并明确移除版本与回滚策略。

## 维护原则

- KISS: 优先最小改动，先修正确性再做重构。
- DRY: 避免重复 DTO、重复解析流程、重复命令处理。
- YAGNI: 不为“可能将来需要”提前引入复杂抽象。
