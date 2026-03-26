# Tools Docs

`tools/` 目录只承载工程脚手架与验证工具；目录内的 `AGENTS.md` / `README.md` 只做入口索引，真实说明统一写在这里。

## 目录分工

- `tools/toolchain/`
  - `python tools/run.py ...` 的主实现
  - dist、verify、tidy、import-gate、log-generator 命令都从这里分发
- `tools/verify/`
  - pipeline runner、checks、snapshot compare 等内部实现
  - 不再作为公开 CLI 入口
- `tools/flows/`
  - 项目级 flow support、底层构建支撑模块
  - 不再作为公开 CLI 入口
- `tools/reporting/`
  - compile2pdf、graph_generator 等报表工具
- `tools/notices/`
  - notices 聚合与生成
- `tools/scripts/`
  - 一次性辅助脚本

## 边界

- `tools/` 放可复用脚手架，不承载业务规则
- `tools/` 不依赖 `tests/`
- 公开 Python 命令统一收口到 `python tools/run.py ...`
- `tools/verify/*`、`tools/flows/*` 现在是内部实现层，不作为公开入口写进操作文档

## 常用入口

- 统一 CLI：
  - `python tools/run.py`
- dist：
  - `python tools/run.py dist <target>`
- verify：
  - `python tools/run.py verify <workflow>`
- Windows 导入门禁：
  - `python tools/run.py import-gate <target> --preset <debug|release|tidy>`
- log_generator：
  - `python tools/run.py log-generator <subcommand>`

## 产物模型

- CMake 产物：
  - `dist/cmake/<target>/<preset>/<scope>/...`
- 测试运行时：
  - `dist/tests/runtime/<project>/workspace/`
  - `dist/tests/runtime/<project>/runs/<run_id>/...`
- 测试归档：
  - `dist/tests/artifact/<project>/latest/...`
  - `dist/tests/artifact/<project>/runs/<run_id>/...`
- pipeline 元数据：
  - `dist/tests/logic/pipeline_runner/<pipeline>/...`

## 进一步阅读

- 总索引：`docs/README.md`
- toolchain SOP：`docs/toolchain_sop.md`
- toolchain 代码地图：`docs/toolchain_tidy_automation.md`
- Windows EXE 编译：`docs/notes/windows_exe_build.md`
- Android APK 编译：`docs/notes/android_apk_build.md`
