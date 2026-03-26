# Tests Docs

`tests/` 目录只承载测试资产；目录内的 `AGENTS.md` / `README.md` 只做入口索引，真实说明统一写在这里。

## 目录分工

- `tests/suites/`
  - 正式测试入口与断言
  - 包括 artifact、logic、toolchain、reporting 等测试套件
- `tests/framework/`
  - 测试运行支撑与共享 helper
- `tests/baseline/`
  - 快照、golden、对比基线
- `tests/generators/`
  - 测试输入生成器
  - 当前重点是 `log_generator`
- `tests/config/`
  - 测试配置样例与默认输入

## 边界

- `tests/` 可以依赖 `tools/` 做构建、验证和断言
- 测试专用支撑代码留在 `tests/`
- 不把通用工程脚手架逻辑堆进 `tests/`
- 构建、dist、verify、pipeline 编排优先落到 `tools/`

## 常用入口

- 统一 Python 入口：
  - `python tools/run.py verify <workflow>`
  - `python tools/run.py dist <target>`
- bills_tracer artifact 测试：
  - `python tests/suites/artifact/bills_tracer/run_tests.py`
- toolchain / reporting unittest：
  - `python -m unittest tests.suites.toolchain.test_verify_cli`
  - `python -m unittest discover -s tests/suites/toolchain`

## 结果读取

- 产物测试 summary：
  - `dist/tests/artifact/<project>/latest/test_summary.json`
- 产物测试日志：
  - `dist/tests/artifact/<project>/latest/logs/`
- 逻辑 / pipeline 输出：
  - `dist/tests/logic/`
- 运行时工作区：
  - `dist/tests/runtime/<project>/workspace/`

## 进一步阅读

- 总索引：`docs/README.md`
- module 级 dist/test 说明：
  - `docs/modules/bills_core/dist_and_test.md`
  - `docs/modules/bills_cli/dist_and_test.md`
  - `docs/modules/bills_io/dist_and_test.md`
  - `docs/modules/bills_android/dist_and_test.md`
