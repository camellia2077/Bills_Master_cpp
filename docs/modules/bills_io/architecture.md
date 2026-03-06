# bills_io Architecture

`libs/bills_io` 是 IO 适配层，负责把 `ports` 抽象连接到具体实现。

## 分层职责

- `adapters/io/`：文本账单文件读取与目录枚举
- `adapters/config/`：TOML 配置文件读取与解析
- `adapters/db/`：sqlite 写入、查询与报表数据网关
- `adapters/reports/`：内建报表 formatter provider 适配
- `io_factory.*`：统一装配默认适配器实现

## 约束

- 不承载业务规则与用例编排
- 业务规则应下沉到 `libs/bills_core`
- 对 core 只通过 `ports` 契约交互
