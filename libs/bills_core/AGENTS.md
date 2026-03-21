# bills_core Agent Guide

`libs/bills_core` 是纯业务主线。这个文件只告诉 agent 先看哪里、边界在哪、最少要跑什么验证。

## Read First

1. `libs/bills_core/README.md`
2. `docs/modules/bills_core/module_map.md`
3. `docs/modules/bills_core/change_guide.md`
4. 改 ABI 时再看 `docs/modules/bills_core/abi_contract.md`

## Use This Module When

- 你在改配置校验、导入、查询、报表、record template 或 ABI
- 你在改纯数据服务和业务规则

## Boundaries

- 不要在这里读写文件、枚举目录、操作 sqlite
- 宿主要读写路径、配置文档或导出文件时，优先去 `libs/bills_io`
- 优先从 `module_map.md` 找主线目录，再去 `rg`

## Verify

- `python tools/verify/verify.py import-layer-check --stats`
- `python tools/verify/verify.py report-consistency-gate`
- `python tools/verify/verify.py core-abi`
- `python tools/verify/verify.py module-mode-check`
