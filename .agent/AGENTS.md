# Project Agent Rules

本文件定义 agent 在本项目中的默认执行方式。

## Terminal

- 终端统一使用 `pwsh`（PowerShell 7.5.4）
- 不要假设默认 shell 是 `cmd`、旧版 `powershell` 或 `bash`
- 需要执行命令时，优先使用适合 `pwsh` 的写法

## Build And Test Entry

- 本项目的 Python 构建、编译、测试入口统一在根目录 `tests/` 相关链路与 `tools/verify/verify.py`
- agent 不需要手工拼接零散编译命令，优先通过 Python 入口触发构建与测试
- Windows 原生静态构建必须使用 MSYS2 `mingw64` 工具链环境
- Windows 下的 `bills-tracer-cli-dist` / `bills-tracer-log-generator-dist` 会默认附带导入表门禁，拒绝第三方 DLL、MinGW runtime DLL 和 `api-ms-win-crt-*`
- 常用入口：
- `python tools/verify/verify.py bills-tracer-cli-dist`
  - `python tools/verify/verify.py bills-tracer`
  - `python tools/verify/verify.py bills-tracer-log-generator-cli-test`
  - `python tools/verify/verify.py bills-tracer-core-dist`
  - `python tools/flows/build_bills_tracer_cli.py --preset debug --scope shared`
  - `python tests/suites/artifact/bills_tracer/run_tests.py`
  - 针对 `tests/suites/toolchain/`、`tests/suites/reporting/` 这类 Python unittest，优先从仓库根目录运行 `python -m unittest ...`
  - 示例：
    - `python -m unittest tests.suites.toolchain.test_verify_cli`
    - `python -m unittest tests.suites.toolchain.test_import_layering`
    - `python -m unittest tests.suites.toolchain.test_boundary_layering`
    - `python -m unittest tests.suites.toolchain.test_pipeline_runner`
  - 其他测试若已接入 `tools/verify/verify.py`，优先走该入口

## Execution Rules

- 修改代码后，优先运行对应的 Python 验证入口
- `tools/` 禁止依赖 `tests/`；`tests/` 可以依赖 `tools/` 做验证与断言
- agent 的职责是：
  - 调用 Python 入口发起构建/测试
  - 读取终端输出
  - 读取测试结果文件或 summary
  - 基于结果继续修复或向用户汇报
- agent 不需要替用户手工操作测试工作区，只需要关注命令输出与最终测试结果

## Android Build Policy

- Android 本地默认验证只编译 `debug` 版本
- 默认命令使用 `python tools/run.py dist bills-tracer-android --preset debug`
- 仅在以下场景主动编译 `release`：
  - 用户明确要求
  - 发布或签名验证
  - shrink/minify/resource shrink 相关问题
  - 仅 `release` 可复现的问题

## Result Reading

- 优先查看终端输出中的失败信息、错误栈、summary
- 若测试流程生成结果文件，优先读取：
  - `dist/tests/artifact/`
  - `dist/tests/logic/`
  - `dist/tests/runtime/`
  - `dist/runtime/`（手工 CLI 默认运行时）
- 若存在 `test_summary.json`、`pipeline_summary.json`、日志文件，优先基于这些结果判断是否通过
- `bills_tracer` CLI artifact 测试的默认 summary 路径是 `dist/tests/artifact/<project>/latest/test_summary.json`
- `bills_tracer` CLI 手工运行默认目录是 `dist/runtime/bills_tracer/workspace/`
- 不要再读取或依赖根目录 `output/`，它是遗留约定

## Temporary Files

- 所有临时文件统一放到根目录 `temp/`
- 不要把临时日志、临时脚本、临时配置写到其他源码目录、文档目录或测试目录
- 若命令或脚本需要生成临时文件，优先显式指定到 `temp/`

## Default Workflow

- 先改代码
- 再运行对应 Python 入口
- 再读取输出与 summary
- 最后只向用户汇报有效结果、失败原因和下一步动作
