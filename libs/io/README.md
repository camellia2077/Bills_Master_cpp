# io

`libs/io` 只承接 IO 适配器和宿主装配。详细说明统一放在 `docs/modules/io/`。

## Start Here

1. `libs/io/AGENTS.md`
2. `docs/modules/io/module_map.md`
3. `docs/modules/io/change_guide.md`
4. `docs/modules/io/dist_and_test.md`

## Quick Pointers

- 找目录和主线：`docs/modules/io/module_map.md`
- 判断改动落点：`docs/modules/io/change_guide.md`
- 看整体边界：`docs/modules/io/architecture.md`

## Quick Verify

- `python tools/run.py verify boundary-layer-check --stats`
