# bills_io Architecture

`libs/bills_io` 是 IO 适配层，负责把 `ports` 抽象接到具体实现。

## 分层职责

- `adapters/io/`：文本文件枚举与读取
- `adapters/config/`：JSON 配置读取与解析
- `adapters/db/`：sqlite 相关数据写入与查询
- `adapters/plugins/`：动态插件加载与 provider 适配
- `io_factory.*`：组合根工厂，集中创建适配实现

## 约束

- 不承载业务规则与用例编排
- 不修改 `libs/bills_core` 的领域语义
- 对 core 只通过 `ports` 契约交互
