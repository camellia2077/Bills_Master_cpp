# bills_io

本目录仅保留 IO 适配源码与导航入口，详细文档统一在 `docs/`。

## 建议阅读顺序

- `libs/bills_io/AGENTS.md`
- `docs/modules/bills_io/README.md`
- `docs/modules/bills_io/module_map.md`

## 详细文档入口

- 总览与阅读顺序：`docs/modules/bills_io/README.md`
- 代码定位：`docs/modules/bills_io/module_map.md`
- 改动落点：`docs/modules/bills_io/change_guide.md`
- 架构说明：`docs/modules/bills_io/architecture.md`
- dist 与验证：`docs/modules/bills_io/dist_and_test.md`
- 通用模块规范：`docs/modules/module_standards.md`

## 常见问题先看哪里

- 改 TOML / 文件 / sqlite 适配器：`docs/modules/bills_io/change_guide.md`
- 找目录与文件：`docs/modules/bills_io/module_map.md`
- 跑验证：`docs/modules/bills_io/dist_and_test.md`

## 本地验证

- 统一入口：`python tools/verify/verify.py`
