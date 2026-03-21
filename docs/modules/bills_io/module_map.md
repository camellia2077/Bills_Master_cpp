# bills_io Module Map

本页用于快速定位 `bills_io` 当前真实主线。

## 代码目录

- `libs/bills_io/src/bills_io/adapters/config/`
  - 配置文档读取与解析
- `libs/bills_io/src/bills_io/adapters/io/`
  - `SourceDocument` 输入、JSON 文件读写、输出路径辅助
- `libs/bills_io/src/bills_io/adapters/db/`
  - sqlite 仓储、查询与报表数据网关
- `libs/bills_io/src/bills_io/adapters/reports/`
  - 报表导出落地
- `libs/bills_io/src/bills_io/host_flow_support.*`
  - CLI / Android 共用宿主准备 helper

## 改动定位建议

- 改配置文档读取：先看 `adapters/config/`
- 改源文档读写、JSON 文件、输出路径：先看 `adapters/io/`
- 改 sqlite 行为：先看 `adapters/db/`
- 改报表文件导出：先看 `adapters/reports/`
- 改宿主共享准备逻辑：先看 `host_flow_support.*`
