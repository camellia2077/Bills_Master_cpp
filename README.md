# Bills Master C++

这是仓库重构阶段的根入口文档。

## 目录入口

- `.github/`：协作与流水线配置（workflow、issue/pr 模板）。
- `cmake/`：跨模块 CMake 基础能力（后续集中到根级）。
- `docs/`：文档中心。
- `apps/`：产品可执行项目（如 `bills_cli`）。
- `libs/`：核心库与领域能力（如 `bills_core`）。
- `testdata/`：跨端共享的标准输入数据（当前 canonical bills 数据集）。
- `dist/`：仓库生成产物根目录（`cmake/`、`tests/`，以及预留给 Android 的 `gradle/`）。
- `tools/`：工程脚本与工具（`toolchain/`、`verify/`、`flows/`）。
- `tests/`：统一测试资产（`suites/`、`baseline/`、`config/`）。
- `third_party/`：第三方源码与依赖镜像（后续接入）。

## 治理文件

- `CONTRIBUTING.md`
- `SECURITY.md`
- `CODE_OF_CONDUCT.md`
- `CHANGELOG.md`

## 当前状态

- 目录重构按 `temp/folder-structure.md` 线性推进。
- `Phase 0` 基线快照已记录在 `temp/phase0_baseline_snapshot.md`。
- 文档总入口：`docs/README.md`。
- clang-tidy / toolchain SOP：`docs/toolchain_sop.md`。
