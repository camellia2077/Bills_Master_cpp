# tests Agent Guide

`tests/` 是仓库里测试资产的总入口。这个文件只做分流；真实说明统一看 `docs/tests/README.md`。

## Read First

1. 先区分改动属于哪一类：
   - 测试入口与断言：`tests/suites`
   - 测试运行支撑：`tests/framework`
   - 快照与 golden：`tests/baseline`
   - 测试输入生成：`tests/generators`
   - 测试配置样例：`tests/config`
2. 再阅读：
   - `docs/tests/README.md`
   - `docs/README.md`

## Use This Area When

- 你在改测试断言、测试流程或回归覆盖
- 你在改测试运行支撑代码
- 你在改快照、golden 或测试输入生成器

## Read More

- `docs/tests/README.md`
- `docs/README.md`
