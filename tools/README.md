# tools

`tools/` 是仓库里工程脚手架、构建、验证、工具链和辅助脚本的总入口。这个目录本身不放实现细节说明，只做分流和索引。

## Start Here

1. `tools/AGENTS.md`
2. `docs/README.md`
3. `docs/toolchain_sop.md`
4. `docs/toolchain_tidy_automation.md`

## Quick Pointers

- `tools/verify/`：统一验证入口与 TOML pipeline 运行入口
- `tools/toolchain/`：统一 Python toolchain 与 tidy SOP 实现
- `tools/flows/`：项目级 build/test flow 脚本
- `tools/reporting/`：compile2pdf 与 graph_generator 等报表辅助工具
- `tools/notices/`：notices 聚合与生成
- `tools/scripts/`：一次性或人工辅助脚本

## Common Entry Points

- `python tools/verify/verify.py`
- `python tools/run.py`

