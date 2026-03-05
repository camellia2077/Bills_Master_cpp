# bills_io Module Map

本页用于快速定位 `libs/bills_io` 的改动入口。

## 代码目录

- `libs/bills_io/src/bills_io/io_factory.hpp`
  - 外部装配入口，优先从这里看“如何注入”
- `libs/bills_io/src/bills_io/adapters/io/`
  - 文件读取、文件枚举适配器
- `libs/bills_io/src/bills_io/adapters/config/`
  - `validator/modifier` 配置加载适配器
- `libs/bills_io/src/bills_io/adapters/db/`
  - sqlite 写入、查询、网关适配器
- `libs/bills_io/src/bills_io/adapters/plugins/common/`
  - 插件加载与 formatter provider 适配

## 改动定位建议

- 改文件输入：先看 `adapters/io/*`
- 改配置读取：先看 `adapters/config/*`
- 改数据库行为：先看 `adapters/db/*`
- 改插件加载：先看 `adapters/plugins/common/*`
- 改装配入口：先看 `io_factory.cpp`
