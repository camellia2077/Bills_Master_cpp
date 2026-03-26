# libs

`libs/` 是仓库里核心业务与 IO 适配层的总入口。这个目录本身不放细节说明，只做模块分流。

## Start Here

1. `libs/AGENTS.md`
2. 如果你在改业务规则、导入、查询、报表、ABI：
   - `libs/bills_core/README.md`
   - `libs/bills_core/AGENTS.md`
3. 如果你在改配置文档读取、源文档 IO、sqlite、导出落地：
   - `libs/io/README.md`
   - `libs/io/AGENTS.md`

## Quick Pointers

- 需要找目录和主线时，优先看对应模块的 `docs/modules/*/module_map.md`
- 需要判断改动落点时，优先看对应模块的 `docs/modules/*/change_guide.md`
