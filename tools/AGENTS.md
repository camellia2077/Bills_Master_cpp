# tools Agent Guide

`tools/` 是仓库里可复用工程脚手架的总入口。这个文件只做分流；真实说明统一看 `docs/tools/README.md`。

## Read First

1. 先判断改动落在哪个一级目录：
   - 验证、workflow 路由、pipeline：`tools/verify`
   - Python toolchain、tidy SOP：`tools/toolchain`
   - 项目级 build/test flow：`tools/flows`
   - reporting 辅助工具：`tools/reporting`
   - notices 聚合：`tools/notices`
   - 一次性辅助脚本：`tools/scripts`
2. 再阅读：
   - `docs/tools/README.md`
   - `docs/README.md`

## Use This Area When

- 你在改统一验证入口、pipeline runner 或 checks
- 你在改 `python tools/run.py ...` 的 toolchain 行为
- 你在改项目构建、测试编排 flow
- 你在改 reporting、notices 或人工辅助脚本

## Read More

- `docs/tools/README.md`
- `docs/README.md`
