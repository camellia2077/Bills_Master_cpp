# bills_core ABI Contract

## 定位

`libs/core/src/abi/bills_core_abi.cpp` 现在是纯数据 ABI 入口。

ABI 不再负责：

- 扫描目录
- 读取 TOML
- 读取 / 写入文本或 JSON 文件
- 拼装 `input_path` / `output_dir` / `config_dir`

这些宿主职责统一交给 `io` 或调用方。

## 输入模型

- 请求根仍为 JSON 对象：
  - `command`
  - `params`
- 记录 / 导入 / 查询主线统一通过文档批次传输：
  - `documents: [{ "display_path": "...", "text": "..." }]`
- 配置主线统一通过配置文档对象传输：
  - `validator_document`
  - `modifier_document`
  - `export_formats_document`

## 当前命令

- `version`
- `capabilities`
- `ping`
- `validate_config_bundle`
- `template_generate`
- `validate_record_batch`
- `preflight_import`
- `validate`
- `convert`
- `ingest`
- `import`
- `query`

## 返回模型

所有响应统一包含：

- `ok`
- `code`
- `message`
- `data`
- `error_layer`
- `abi_version`
- `response_schema_version`
- `error_code_schema_version`

## 边界规则

- ABI 只做参数解包、调用纯数据服务、结果封装
- 不再承诺旧路径型字段兼容
- 若命令需要配置校验、文档读取或产物写回，应由调用方先经 `io` 处理

## 变更检查清单

1. 是否新增或删除纯数据字段
2. 是否同步更新 `capabilities` 的命令列表
3. 是否需要补充调用方侧集成验证
