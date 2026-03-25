# tests Agent Guide

`tests/` 是仓库里测试资产的总入口。这个文件只告诉 agent 先去哪一类测试目录，再进入已有 docs 看细节。

## Read First

1. 先区分改动属于哪一类：
   - 测试入口与断言：`tests/suites`
   - 测试运行支撑：`tests/framework`
   - 快照与 golden：`tests/baseline`
   - 测试输入生成：`tests/generators`
   - 测试配置样例：`tests/config`
2. 再按需要阅读：
   - `docs/modules/bills_core/dist_and_test.md`
   - `docs/README.md`

## Use This Area When

- 你在改测试断言、测试流程或回归覆盖
- 你在改测试运行支撑代码
- 你在改快照、golden 或测试输入生成器

## Boundaries

- `tests/` 可以依赖 `tools/` 做验证
- 测试专用支撑代码留在 `tests/`
- 不把通用脚手架逻辑堆进 `tests/`

## Common Entry Points

- `python tools/verify/verify.py`
- `python tests/suites/artifact/bills_tracer/run_tests.py`
- `python -m unittest tests.suites.toolchain.test_verify_cli`
- `python -m unittest discover -s tests/suites/toolchain`

## Read More

- `docs/modules/bills_core/dist_and_test.md`
- `docs/README.md`
