# bills_io Agent Guide

`libs/bills_io` 是跨端 IO 与宿主装配层。这个文件只保留 agent 需要的最小导航和边界。

## Read First

1. `libs/bills_io/README.md`
2. `docs/modules/bills_io/module_map.md`
3. `docs/modules/bills_io/change_guide.md`
4. 需要看整体边界时再看 `docs/modules/bills_io/architecture.md`

## Use This Module When

- 你在改配置文档读取、源文档输入、JSON 文件读写、sqlite 仓储、导出落地
- 你在改 CLI/Android 的共享宿主拼装

## Boundaries

- 这里不承载业务规则
- 业务规则优先去 `libs/bills_core`
- 不要让 `bills_io` 反向依赖表现层
- 优先从 `module_map.md` 找目录，再去 `rg`

## Verify

- `python tools/verify/verify.py import-layer-check --stats`
- `python tools/verify/verify.py boundary-layer-check --stats`
