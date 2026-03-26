# Windows EXE 编译说明

本文档说明当前仓库里推荐的 Windows 可执行文件编译命令，以及 `debug` / `release` 两种常用构建方式。

## 1. 前提

Windows 原生静态构建必须使用 MSYS2 `mingw64` 工具链环境：

- `C:/msys64/mingw64/bin/clang.exe`
- `C:/msys64/mingw64/bin/clang++.exe`
- `C:/msys64/mingw64/bin/ar.exe`
- `C:/msys64/mingw64/bin/ranlib.exe`

当前仓库已经收口为 `clang` 单编译器，不再支持 `gcc`。

## 2. 推荐命令

统一使用 `python tools/run.py`：

- 编译 debug 版 exe：
  - `python tools/run.py dist bills-tracer-cli --preset debug --scope shared`
- 编译 release 版 exe：
  - `python tools/run.py dist bills-tracer-cli --preset release --scope shared`
- 编译后执行 Windows 导入表门禁：
  - `python tools/run.py import-gate bills-tracer-cli --preset debug --scope shared`
  - `python tools/run.py import-gate bills-tracer-cli --preset release --scope shared`

其中：

- `python tools/run.py dist bills-tracer-cli --preset ...`
  - 适合日常本地编译
- `python tools/run.py import-gate bills-tracer-cli --preset ...`
  - 适合验证最终分发形态

## 3. 产物位置

CMake 原始输出：

- debug：
  - `dist/cmake/bills_tracer_cli/debug/shared/bin/bills_tracer_cli.exe`
- release：
  - `dist/cmake/bills_tracer_cli/release/shared/bin/bills_tracer_cli.exe`

默认同步后的 runtime 工作区：

- `dist/runtime/bills_tracer/workspace/bills_tracer_cli.exe`

如果你是拿来直接运行，通常更建议使用 runtime 工作区那份，因为脚本会同时同步：

- `config/`
- `notices/`

## 4. release 说明

`release` 版本默认开启仓库当前定义的发布优化参数，并保持 Windows 静态链接策略。

如果你只是本地调试功能或排查崩溃，优先编译 `debug`；如果你要验证最终分发形态，优先编译 `release` 并额外执行一次 `python tools/run.py import-gate bills-tracer-cli --preset release --scope shared`。
