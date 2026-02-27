# bills_core ABI Contract

## ABI 层职责

- 接收外部命令参数。
- 调用 core/use case。
- 统一输出状态码、错误信息、结果 payload。

## 约束边界

- ABI handler 不承载业务规则，仅做编排与映射。
- 参数校验失败与业务执行失败要区分错误类别。
- 返回结构保持向后兼容；新增字段优先可选。

## 模块化建议

- 按命令拆分 handler（如 `validate/convert/import/query/export`）。
- 每个 handler 独立 `.cpp` 编译单元，避免单文件过大回归风险。

## 变更检查清单

1. 是否影响现有命令入参与出参。
2. 是否更新错误码或错误文本映射。
3. 是否通过 `python scripts/verify.py`。
4. 是否需要更新调用方文档或 schema。

