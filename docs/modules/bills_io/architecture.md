# bills_io Architecture

`libs/bills_io` 是 IO 与宿主装配层，负责把路径、文件、sqlite 和导出落地接到 `bills_core` 的纯数据服务上。

## 当前主线

- `adapters/config/`
  - 配置文档读取与 TOML -> 文档 DTO
- `adapters/io/`
  - 源文档批次读取、文本写回、JSON 文件读写、输出路径辅助
- `adapters/db/`
  - sqlite 仓储、查询和报表数据网关
- `adapters/reports/`
  - 报表导出落地
- `host_flow_support.*`
  - CLI / Android 共用的宿主准备 helper

## 边界

- 这里不承载业务规则
- 需要改“规则是什么”时，回到 `libs/bills_core`
- 需要改“路径怎么变成数据、数据怎么落回文件”时，优先看这里
