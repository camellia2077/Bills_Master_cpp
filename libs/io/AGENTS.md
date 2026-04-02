# io Agent Guide

`libs/io` 是跨端 IO 与宿主装配层。这个文件只保留 agent 需要的最小导航和边界。

## Read First

1. `libs/io/README.md`
2. `docs/modules/io/module_map.md`
3. `docs/modules/io/change_guide.md`
4. 需要看整体边界时再看 `docs/modules/io/architecture.md`

## Use This Module When

- 你在改配置文档读取、源文档输入、JSON 文件读写、sqlite 仓储、导出落地
- 你在改 CLI/Android 的共享宿主拼装

## Boundaries

- 这里不承载业务规则
- 业务规则优先去 `libs/core`
- 不要让 `io` 反向依赖表现层
- 优先从 `module_map.md` 找目录，再去 `rg`

## Verify

- `python tools/run.py verify import-layer-check --stats`
- `python tools/run.py verify boundary-layer-check --stats`
