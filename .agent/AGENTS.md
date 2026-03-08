# Project Agent Rules

本文件定义 agent 在本项目中的默认执行方式。

## Terminal

- 终端统一使用 `pwsh`（PowerShell 7.5.4）
- 不要假设默认 shell 是 `cmd`、旧版 `powershell` 或 `bash`
- 需要执行命令时，优先使用适合 `pwsh` 的写法

## Build And Test Entry

- 本项目的 Python 构建、编译、测试入口统一在根目录 `tests/` 相关链路与 `tools/verify/verify.py`
- agent 不需要手工拼接零散编译命令，优先通过 Python 入口触发构建与测试
- 常用入口：
- `python tools/verify/verify.py bills-build`
  - `python tools/verify/verify.py bills`
  - `python tools/verify/verify.py log-cli-test`
  - 其他测试若已接入 `tools/verify/verify.py`，优先走该入口

## Execution Rules

- 修改代码后，优先运行对应的 Python 验证入口
- agent 的职责是：
  - 调用 Python 入口发起构建/测试
  - 读取终端输出
  - 读取测试结果文件或 summary
  - 基于结果继续修复或向用户汇报
- agent 不需要替用户手工操作测试工作区，只需要关注命令输出与最终测试结果

## Result Reading

- 优先查看终端输出中的失败信息、错误栈、summary
- 若测试流程生成结果文件，优先读取：
  - `build/tests/`
  - `build/tests/artifact/`
  - `build/tests/logic/`
  - `build/tests/runtime/`
- 若存在 `test_summary.json`、`pipeline_summary.json`、日志文件，优先基于这些结果判断是否通过

## Temporary Files

- 所有临时文件统一放到根目录 `temp/`
- 不要把临时日志、临时脚本、临时配置写到其他源码目录、文档目录或测试目录
- 若命令或脚本需要生成临时文件，优先显式指定到 `temp/`

## Default Workflow

- 先改代码
- 再运行对应 Python 入口
- 再读取输出与 summary
- 最后只向用户汇报有效结果、失败原因和下一步动作
