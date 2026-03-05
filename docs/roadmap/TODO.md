# TODO / Roadmap

本文件是 `docs` 下唯一待办入口，已合并历史 `TODO.md` 与 `todo1.md` 内容。
历史原文归档：`docs/release/todo1_legacy.md`。

## P0: 紧急修复

- 暂无。

## P1: 高优先级

- [ ] 评估并引入 `cjson` 方案，支持按构建目标切换 JSON 实现。

## P2: 中优先级

- [ ] 使用 JSON 配置导出 `latex` 字体格式。
- [ ] 给 `parent_item` 增加注释字段。
- [ ] 给 `Modifier_Config` 配置文件增加合法性校验。
- [ ] 在导出报告中增加 `source/comment/transaction_type` 字段开关。
- [ ] 为年度报告增加月度平均值与差异分析。

## P3: 低优先级（重构方向）

- [ ] 稳定层级命名：`application/use_cases`、`ports`、`adapters`。
- [ ] 拆分 presentation 层：`presentation/commands`、`presentation/controllers`。
- [ ] 统一入口层错误模型：从 CLI 到 use case 统一 `Result/Error`。
- [ ] 归并配置校验到 `adapters/config`。
- [ ] 拆分 reports 查询与格式化端口（`QueryPort/FormatterPort`）。
- [ ] 清理命名与冗余实现，收口历史 include 路径。

## 执行检查清单

- [ ] CMake 源文件列表已同步更新。
- [ ] include 路径从 `src` 根可稳定解析。
- [ ] controller 仅负责装配与调用 use case。
- [ ] ports/adapters 无反向依赖。
