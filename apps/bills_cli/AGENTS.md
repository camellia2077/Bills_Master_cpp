# bills_cli Agent Guide

`apps/bills_cli` 是 CLI 表现层与组合根。这个文件只给 agent 提供最小导航、边界和验证入口。

## Read First

1. `apps/bills_cli/README.md`
2. `docs/modules/bills_cli/module_map.md`
3. `docs/modules/bills_cli/change_guide.md`
4. 需要看整体关系时再看 `docs/modules/bills_cli/architecture.md`

## Use This Module When

- 你在改 CLI 命令协议、命令分组或帮助输出
- 你在改 CLI 到 `bills_core` / `io` 的宿主装配
- 你在改 CLI 测试脚本或手工运行入口

## Boundaries

- 这里不定义核心业务规则
- 业务规则和纯数据服务优先去 `libs/bills_core`
- 文件、配置、数据库和导出落地优先去 `libs/io`
- 改 CLI 时优先从 `docs/modules/bills_cli/module_map.md` 找目录，再去 `rg`

## Verify

- `python tools/verify/verify.py bills`
- 如果只改文档，可跳过代码验证
