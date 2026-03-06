# bills_io Module Map

本页用于快速定位 `libs/bills_io` 的代码入口。

## 代码目录

- `libs/bills_io/src/bills_io/io_factory.hpp`
  - 默认装配入口
- `libs/bills_io/src/bills_io/adapters/io/`
  - 文件读取、文件枚举适配器
- `libs/bills_io/src/bills_io/adapters/config/`
  - `validator/modifier` 的 TOML 配置加载适配器
- `libs/bills_io/src/bills_io/adapters/db/`
  - sqlite 写入、查询、数据网关适配器
- `libs/bills_io/src/bills_io/adapters/reports/`
  - 内建报表 formatter provider 适配器

## 改动定位建议

- 改文件输入：先看 `adapters/io/*`
- 改配置读取：先看 `adapters/config/*`
- 改数据库行为：先看 `adapters/db/*`
- 改报表 formatter 装配：先看 `adapters/reports/*`
- 改装配入口：先看 `io_factory.cpp`
