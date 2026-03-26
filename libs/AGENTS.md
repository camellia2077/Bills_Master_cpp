# libs Agent Guide

`libs/` 是核心业务和 IO 适配层的总入口。这个文件只负责告诉 agent 先去哪一个模块，再去看各模块自己的薄链接文档。

## Read First

1. 先判断改动属于哪个模块：
   - 业务规则、导入、查询、报表、ABI：`libs/bills_core`
   - 配置文档读取、源文档 IO、sqlite、导出落地：`libs/io`
2. 进入对应模块后，先读：
   - `libs/<module>/README.md`
   - `libs/<module>/AGENTS.md`
3. 再按需要看：
   - `docs/modules/bills_core/module_map.md`
   - `docs/modules/io/module_map.md`

## Boundaries

- `bills_core` 只承接纯业务规则
- `io` 只承接 IO 适配和宿主装配
- 表现层改动优先去 `apps/bills_cli` 或 `apps/bills_android`

## Verify

- `python tools/verify/verify.py`
- 纯文档改动可跳过代码验证
