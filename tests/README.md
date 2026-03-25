# tests

`tests/` 是仓库里测试套件、测试框架、基线、生成器和测试配置的总入口。这个目录本身不承载细节说明，只做测试资产分流。

## Start Here

1. `tests/AGENTS.md`
2. `docs/modules/bills_core/dist_and_test.md`

## Quick Pointers

- `tests/suites/`：正式测试入口
- `tests/framework/`：测试运行支撑
- `tests/baseline/`：快照与 golden
- `tests/generators/`：测试输入生成器
- `tests/config/`：测试配置样例

## Common Entry Patterns

- bills_tracer artifact 入口：`python tests/suites/artifact/bills_tracer/run_tests.py`
- toolchain / reporting unittest：`python -m unittest ...`
