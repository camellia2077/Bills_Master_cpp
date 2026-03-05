# bills_cli Agent Guide

`apps/bills_cli` 目录只保留入口约束，详细说明统一在 `docs/modules/bills_cli/`。

## 必读顺序

1. `apps/bills_cli/README.md`
2. `docs/modules/bills_cli/module_map.md`
3. `docs/modules/bills_cli/change_guide.md`

## 约束

- `bills_cli` 作为组合根与表现层，不新增核心业务规则
- 业务编排应复用 `libs/bills_core` 的 use case / ports
- IO 与插件实现优先复用 `libs/bills_io`

## 验证

- 修改后至少执行一次：`python tools/verify/verify.py`
- 失败需修复后重跑，直到返回码 `0`
- 仅当本次改动为纯文档修改（`docs/**`、`*.md`）时，可跳过 `verify`
