# Legacy Pipeline 下线计划（Phase 4）

## 背景
- `legacy` 导出管线已转为过渡模式，仅保留短期回退用途。
- 从 `2026-03-05` 起，CLI 在使用 `legacy` 时会输出弃用警告。
- 目标下线日期：`2026-06-30`。
- 当前导出落点已不在 `bills_core`，由 `libs/bills_io/src/bills_io/adapters/reports/report_export_service.cpp` 负责宿主侧导出编排。

## 当前策略
- 标准格式统一走标准渲染链：
  - `json` / `md` / `tex`
- legacy 插件回退仅保留非标准格式（如 `rst` 或自定义插件）场景。
- 不再为新格式新增 legacy 专属分叉逻辑。

## 下线前置条件
- 连续通过一致性门禁：`python tools/run.py verify report-consistency-gate`
- 回归窗口内无阻塞级问题依赖 `legacy` 回退。
- `model-first`（默认）与 `json-first` 在业务数据样本上稳定一致。

## 执行步骤
1. 冻结当前 `legacy` 使用清单（命令、脚本、外部调用方）。
2. 对调用方逐步迁移到 `model-first`（默认）或 `json-first`。
3. 保持每次发布前执行一致性门禁并记录结果。
4. 到达下线日期且满足前置条件后，移除 `legacy` 入口与相关提示文案。

## 风险与回滚
- 风险：极少数自定义插件可能仍依赖 legacy 路径。
- 缓解：在下线窗口内提供 `json-first` 对照与门禁报告。
- 回滚：若发生阻塞问题，临时恢复 legacy 开关，但需补充 RCA 与下线延后说明。
