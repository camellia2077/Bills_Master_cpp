# core

`libs/core` 只承接纯业务规则和纯数据服务。详细说明统一放在 `docs/modules/core/`。

## Start Here

1. `libs/core/AGENTS.md`
2. `docs/modules/core/module_map.md`
3. `docs/modules/core/change_guide.md`
4. 改 ABI 时再看 `docs/modules/core/abi_contract.md`

## Quick Pointers

- 找主线目录：`docs/modules/core/module_map.md`
- 判断改动落点：`docs/modules/core/change_guide.md`
- 看整体边界：`docs/modules/core/architecture.md`

## Quick Verify

- `python tools/verify/verify.py core-dist`
- `python tools/verify/verify.py module-mode-check`
