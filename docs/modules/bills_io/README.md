# bills_io Docs

这是 `libs/bills_io` 的文档落地页，目标是让人或 agent 在进入模块后，先快速确认“该看哪份文档、该改哪类文件”。

## 推荐阅读顺序

1. `libs/bills_io/README.md`
2. `libs/bills_io/AGENTS.md`
3. `docs/modules/bills_io/module_map.md`
4. `docs/modules/bills_io/change_guide.md`
5. 按需要继续看专题文档

## 常用入口

- 代码定位：`docs/modules/bills_io/module_map.md`
- 改动落点：`docs/modules/bills_io/change_guide.md`
- 架构说明：`docs/modules/bills_io/architecture.md`
- dist 与验证：`docs/modules/bills_io/dist_and_test.md`

## 遇到这些需求先看哪里

- 改文件读取 / 目录遍历：
  - `docs/modules/bills_io/change_guide.md`
- 改 TOML 配置解析：
  - `docs/modules/bills_io/change_guide.md`
- 改 sqlite 写入 / 查询 / 数据网关：
  - `docs/modules/bills_io/change_guide.md`
- 想知道源码目录位置：
  - `docs/modules/bills_io/module_map.md`

## 边界提醒

- `libs/bills_io` 只放适配器与装配
- 业务规则应放 `libs/bills_core`
- CLI 层改动应放 `apps/bills_cli`
