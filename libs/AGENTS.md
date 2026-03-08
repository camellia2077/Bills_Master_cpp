# libs Agent Guide

`libs/` 目录用于放核心业务与基础适配模块。这里的 `AGENTS.md` 只负责总导航与边界提醒，详细模块说明继续看各模块自己的 `README.md`、`AGENTS.md` 与 `docs/modules/`。

## 推荐阅读顺序

1. 先确认改动属于哪个模块：
   - 业务规则 / 报表编排 / ABI / ports：`libs/bills_core`
   - IO / TOML 配置读取 / sqlite / formatter provider：`libs/bills_io`
2. 进入对应模块后，先读：
   - `libs/<module>/README.md`
   - `libs/<module>/AGENTS.md`
3. 再看对应 docs 落地页：
   - `docs/modules/bills_core/README.md`
   - `docs/modules/bills_io/README.md`

## 模块边界

- `libs/bills_core`
  - 放业务规则、use case、reports、ABI、ports
  - 不承载平台/CLI 细节
- `libs/bills_io`
  - 放文件、TOML、sqlite、formatter provider 等适配器实现
  - 不承载业务规则
- `apps/bills_cli`
  - 负责表现层与命令分发
  - 不新增核心业务规则

## 修改前先判断

- 如果你在改“规则是什么”，优先去 `libs/bills_core`
- 如果你在改“数据怎么读/写/装配”，优先去 `libs/bills_io`
- 如果你在改 CLI 参数、命令分发或控制器，去 `apps/bills_cli`

## 验证

- 修改 `libs/` 下代码后，至少执行一次：`python tools/verify/verify.py`
- 若本次改动只涉及文档，可跳过代码验证
