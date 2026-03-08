# bills_core Agent Guide

`libs/bills_core` 目录只保留执行入口与导航，详细说明统一维护在 `docs/modules/bills_core/`。

## 必读顺序

1. `libs/bills_core/README.md`
2. `docs/modules/bills_core/README.md`
3. `docs/modules/bills_core/module_map.md`
4. `docs/modules/bills_core/change_guide.md`

## 约束

- 涉及 ABI 改动时，必须先读：`docs/modules/bills_core/abi_contract.md`
- 仅修改需求相关目录，避免跨模块顺手重构
- 优先复用 `ports` / `use_cases` / 已有模块接口，不新增重复实现

## 验证

- 修改后至少执行一次：`python tools/verify/verify.py`
- 失败需修复后重跑，直到返回码 `0`
- 仅当本次改动为纯文档修改（`docs/**`、`*.md`）时，可跳过 `verify`
