# bills_core Architecture

## 目标

`libs/bills_core` 现在只承载平台无关业务能力，目录事实源固定为：

- `common/`
- `config/`
- `domain/`
- `ingest/`
- `query/`
- `record_template/`
- `reporting/`
- `abi/`
- `ports/`
- `modules/`

`bills_core` 不再负责：

- TOML 文件读取或 TOML 解析入口
- 文本 / JSON 文件读写
- 路径拼装与目录枚举
- 导出文件落盘
- sqlite 具体实现

这些职责统一下沉到 `libs/io` 或宿主层。

## 分层职责

- `config/`
  - 配置文档 DTO、配置校验报告、运行时配置装配
- `ingest/`
  - TXT 校验、转换、JSON 编解码、导入批处理主线
- `query/`
  - 基于 `ReportDataGateway` 的查询结果组装
- `record_template/`
  - 模板生成、记录批次预览、period 列举、preflight import
- `reporting/`
  - `StandardReport` DTO、assembler、renderer registry、格式渲染与排序
- `abi/`
  - 纯数据 C ABI 入口，只接收结构化 JSON 参数并返回结果
- `ports/`
  - 核心仍需要的宿主抽象，目前仅保留仓储与查询网关
- `modules/`
  - 与当前 canonical 目录一一对应的模块导出面与 smoke

## 约束

- `bills_core` 源码中禁止出现 `toml::`、`sqlite3`、`<fstream>`、`recursive_directory_iterator`、`create_directories`
- `abi/` 不再接收 `input_path`、`config_dir`、`output_dir`
- `ingest/`、`record_template/`、`abi/` 统一基于 `SourceDocumentBatch` 处理输入
- `reporting/` 只产出字符串或 `StandardReport`，不直接 `std::cout/std::cerr`

## 宿主协作

- `libs/io`
  - 负责 `ConfigDocumentParser`、`SourceDocumentIo`、`JsonBillDocumentIo`、`ReportExportService`
  - 通过 `host_flow_support.*` 为 CLI / Android 提供公共宿主拼装逻辑
- `apps/bills_cli`
  - 做参数解析、运行时目录定位、命令行输出
- `apps/bills_android`
  - 做 JNI 边界、Android 私有工作区与 UI 返回 JSON 封装

## 相关阅读

- `docs/modules/bills_core/module_map.md`
- `docs/modules/bills_core/change_guide.md`
- `docs/modules/bills_core/abi_contract.md`
- `docs/modules/bills_core/dist_and_test.md`
