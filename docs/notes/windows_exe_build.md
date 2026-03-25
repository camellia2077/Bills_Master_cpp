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

优先使用统一入口：

- 编译 debug 版 exe：
  - `python tools/run.py dist bills-tracer-cli --preset debug --scope shared`
- 编译 release 版 exe：
  - `python tools/run.py dist bills-tracer-cli --preset release --scope shared`

这条命令适合日常编译，也符合仓库当前“统一入口优先”的约定。

## 3. 底层脚本

如果你需要直接走底层 flow，可运行：

- 编译 debug 版 exe：
  - `python tools/flows/build_bills_tracer_cli.py --preset debug --scope shared`
- 编译 release 版 exe：
  - `python tools/flows/build_bills_tracer_cli.py --preset release --scope shared`

统一入口和底层脚本是两层关系：

- `python tools/run.py dist bills-tracer-cli --preset ...`
  - 负责提供仓库统一的 dist 入口
- `python tools/flows/build_bills_tracer_cli.py`
  - 负责实际执行 CMake 配置、编译和 runtime 产物同步

## 4. verify 入口

如果你希望编译后顺手跑 Windows 导入表门禁，可直接使用：

- debug：
  - `python tools/verify/verify.py bills-tracer-cli-dist`
- release：
  - `python tools/verify/verify.py bills-tracer-cli-dist -- --preset release --scope shared`

这条入口在 Windows 下会额外检查导入表，拒绝第三方 DLL、MinGW runtime DLL 和 `api-ms-win-crt-*`。

## 5. 产物位置

CMake 原始输出：

- debug：
  - `dist/cmake/bills_tracer_cli/debug/shared/bin/bills_tracer_cli.exe`
- release：
  - `dist/cmake/bills_tracer_cli/release/shared/bin/bills_tracer_cli.exe`

默认同步后的 runtime 工作区：

- `dist/tests/runtime/bills_tracer/workspace/bills_tracer_cli.exe`

如果你是拿来直接运行，通常更建议使用 runtime 工作区那份，因为脚本会同时同步：

- `config/`
- `notices/`

## 6. release 说明

`release` 版本默认开启仓库当前定义的发布优化参数，并保持 Windows 静态链接策略。

如果你只是本地调试功能或排查崩溃，优先编译 `debug`；如果你要验证最终分发形态，优先编译 `release` 并使用 `verify.py bills-tracer-cli-dist` 做一次完整门禁。
