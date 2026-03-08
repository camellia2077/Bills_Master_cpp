# bills_core Docs

这是 `libs/bills_core` 的文档落地页，目标是让人或 agent 在进入模块后，先用一页确定“先读什么、改哪里、怎么验证”。

## 推荐阅读顺序

1. `libs/bills_core/README.md`
2. `libs/bills_core/AGENTS.md`
3. `docs/modules/bills_core/module_map.md`
4. `docs/modules/bills_core/change_guide.md`
5. 按需要继续看专题文档

## 常用入口

- 代码定位：`docs/modules/bills_core/module_map.md`
- 改动落点：`docs/modules/bills_core/change_guide.md`
- 架构说明：`docs/modules/bills_core/architecture.md`
- dist 与验证：`docs/modules/bills_core/dist_and_test.md`
- ABI 契约：`docs/modules/bills_core/abi_contract.md`

## 遇到这些需求先看哪里

- 改账单转换、校验、报表规则：
  - `docs/modules/bills_core/change_guide.md`
- 想知道目录和文件分布：
  - `docs/modules/bills_core/module_map.md`
- 改 ABI 或对外 JSON 结构：
  - `docs/modules/bills_core/abi_contract.md`
  - `docs/modules/bills_core/standard_report_json_schema_v1.md`
- 改 dist 或测试链路：
  - `docs/modules/bills_core/dist_and_test.md`

## 边界提醒

- 业务规则留在 `libs/bills_core`
- IO / sqlite / TOML 适配器优先看 `libs/bills_io`
- CLI 命令分发优先看 `apps/bills_cli`
