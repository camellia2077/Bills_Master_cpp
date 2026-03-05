# bills_io Agent Guide

`libs/bills_io` 目录只保留入口约束，详细说明统一在 `docs/modules/bills_io/`。

## 必读顺序

1. `libs/bills_io/README.md`
2. `docs/modules/bills_io/module_map.md`
3. `docs/modules/bills_io/change_guide.md`

## 约束

- `bills_io` 仅实现适配器，不承载业务规则
- 业务规则应落在 `libs/bills_core`
- 通过 `ports` 契约对接，不反向依赖 presentation

## 验证

- 修改后至少执行一次：`python tools/verify/verify.py`
- 失败需修复后重跑，直到返回码 `0`
