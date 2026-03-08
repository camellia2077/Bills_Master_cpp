# bills_tracer Toolchain / Clang-Tidy SOP

本文档描述当前仓库中 `clang-tidy` 自动化批处理的推荐工作流、状态语义和排障入口。

如果你想快速定位 Python 实现代码，而不是只看命令怎么用，请先看：

- `docs/toolchain_tidy_automation.md`

## 1. 统一入口

- CLI 总入口：`python tools/run.py`
- 统一验证入口：`python tools/verify/verify.py`
- toolchain 配置：`tools/toolchain/config/workflow.toml`
- tidy 运行目录：`temp/tidy/`
- canonical state：
  - `temp/tidy/state/latest.json`
  - `temp/tidy/state/batches/<batch_id>.json`
- legacy projection：
  - `temp/tidy/tidy_result.json`
  - `temp/tidy/batch_status.json`
  - `temp/tidy/tidy_batch_checkpoint.json`

## 2. 推荐完整流程

### 2.1 初始化队列

1. `python tools/run.py format --check`
2. `python tools/run.py tidy`
3. `python tools/run.py tidy-split`
4. `python tools/run.py tidy-status`

这一步的目标是生成原始 tidy 日志、切分任务、建立 `temp/tidy/tasks/` 队列。

### 2.2 按批次执行 SOP

主入口固定为：

- `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop`

不要再把下面这些步骤交给 agent 手工串：

- 手工读 `temp/tidy/tidy_result.json`
- 手工跑 baseline verify
- 手工跑 `tidy-fix`
- 手工拼 targeted recheck
- 手工从 log 摘剩余 warnings
- 手工解释为什么下一个 batch 是 `batch_013`

这些步骤现在都应由 `tidy-batch --preset sop` 或只读诊断命令负责。

### 2.3 批次完成后查看状态

- 全局状态：`python tools/run.py tidy-status`
- 单批状态：`python tools/run.py tidy-status --batch-id <BATCH_ID>`
- 查看批次任务与相位：`python tools/run.py tidy-show --batch-id <BATCH_ID>`
- 仅重跑当前批 recheck：`python tools/run.py tidy-recheck --batch-id <BATCH_ID>`

### 2.4 全部批次完成后

- `python tools/run.py tidy-close`

## 3. `tidy-batch --preset sop` 的真实阶段

当前 SOP 固定为 8 个 phase：

1. `load_queue`
2. `verify`
3. `safe_fix_prepass`
4. `build_gate`
5. `recheck`
6. `classify`
7. `clean_refresh`
8. `finalize`

### 3.1 `load_queue`

- 读取 `temp/tidy/tasks_manifest.json`
- 确定 batch 对应的 `source_files`
- 刷新该 batch 的编号上下文 `numbering_context`
- 清空上一轮遗留的 `build_gate / recheck / remaining / decision_summary`

### 3.2 `verify`

- 默认执行 baseline verify：
- `python tools/verify/verify.py bills-build`
- 结果写入：
  - `temp/tidy/verify_result.json`
  - batch phase state

### 3.3 `safe_fix_prepass`

- 调用 `tidy-fix`
- 只允许安全白名单 checks 生效
- 当前实现会强制从 `-*` 起步，再附加白名单 checks，避免被仓库 `.clang-tidy` 的广域配置污染

默认白名单在 `workflow.toml` 中配置，典型包括：

- `modernize-use-nullptr`
- `modernize-use-override`
- `modernize-use-using`
- `modernize-loop-convert`
- `modernize-raw-string-literal`
- `readability-braces-around-statements`
- `readability-else-after-return`
- `readability-redundant-control-flow`
- `modernize-use-trailing-return-type`
- `modernize-use-nodiscard`

明确不应在 prepass 中自动放开：

- `readability-identifier-naming`
- `modernize-pass-by-value`
- `bugprone-easily-swappable-parameters`

### 3.4 `build_gate`

- 在 prepass 之后立即再次执行：
- `python tools/verify/verify.py bills-build`
- 如果失败：
  - batch 状态写成 `needs_manual_after_fix`
  - `suspect_files` 指向本轮 prepass 改动过的文件
  - 后续 `recheck / classify / clean_refresh / finalize` 会被跳过
  - 同时清空旧 `recheck / remaining / decision_summary`，避免脏状态误导后续 agent

### 3.5 `recheck`

- 只针对当前 batch 的去重 `source_files` 做 targeted clang-tidy
- 输出：
  - `temp/tidy/refresh/<batch_id>_recheck.log`
  - `temp/tidy/refresh/<batch_id>_recheck.diagnostics.jsonl`

### 3.6 `classify`

- 解析 recheck 日志
- 产出结构化 residual diagnostics
- 每条 residual 至少包含：
  - `file`
  - `line`
  - `col`
  - `check`
  - `message`
  - `severity`
  - `preferred_action`
  - `fallback_action`
  - `reason_template`

动作语义：

- `preferred_action = suggest_nolint`
  - 仅限 suppression allowlist 中的 checks
- `preferred_action = manual_refactor`
  - 其余 residual checks
- `fallback_action = suggest_nolint`
  - 仅补充给：
    - `readability-function-cognitive-complexity`
    - `bugprone-easily-swappable-parameters`

注意：

- 当前策略是 `suggest_only`
- 不自动往源码写 `NOLINTNEXTLINE`

### 3.7 `clean_refresh`

- 只有当 `recheck` 后无 residual 才继续
- 执行 `clean --strict`
- 执行 `tidy-refresh`

### 3.8 `finalize`

- 刷新 queue / latest state
- 更新兼容投影视图
- 推进到下一个 open batch

## 4. 状态文件怎么读

### 4.1 canonical state 是唯一事实来源

- 全局：`temp/tidy/state/latest.json`
- 单批：`temp/tidy/state/batches/<batch_id>.json`

这些文件是新的事实来源；其他旧 JSON 只是 projection。

### 4.2 全局状态里最重要的字段

- `queue`
- `current_batch`
- `current_phase`
- `next_action`
- `numbering_context`
- `last_verify`
- `last_run`

### 4.3 单批状态里最重要的字段

- `source_files`
- `phases`
- `auto_fix_prepass`
- `build_gate`
- `recheck`
- `remaining`
- `decision_summary`
- `numbering_context`

## 5. 批次编号语义

`numbering_context` 用来解释以下问题：

- 当前 batch 是哪个
- 为什么下一批是 `batch_013`
- 为什么 `batch_001..batch_012` 被认为已经关闭

它固定包含：

- `current_batch`
- `already_closed_before_current`
- `already_closed_ranges`
- `next_open_batch`

推荐查看命令：

- `python tools/run.py tidy-status`
- `python tools/run.py tidy-status --batch-id <BATCH_ID>`

不要再人工猜测“是不是某些批次被跳过了”；先以 `numbering_context` 为准。

## 6. agent 工作约定

### 6.1 推荐只读入口

- `python tools/run.py tidy-status`
- `python tools/run.py tidy-status --batch-id <BATCH_ID>`
- `python tools/run.py tidy-show --batch-id <BATCH_ID>`

### 6.2 只有排障时才看原始日志

只有在以下场景才建议直接读原始 log / JSON：

- 怀疑 log parser 漏解析
- 怀疑 safe-fix 误改源码
- 怀疑 `build_gate` / `recheck` 结果写入错误
- 在开发 toolchain 本身

优先级应始终是：

1. `tidy-status`
2. `batch state`
3. `tidy-show`
4. 原始 `.log / .jsonl`

### 6.3 不再推荐的手工动作

- 手工拼 `clang-tidy` recheck 命令
- 手工统计 residual checks 数量
- 手工解释批次编号状态
- 手工决定是否存在“unexpected safe-fix residual”

## 7. 常见状态解释

### `needs_manual`

- `safe_fix_prepass` 和 `build_gate` 已通过
- recheck 后仍有 residual
- 需要人工重构或根据策略建议添加窄 suppression

### `needs_manual_after_fix`

- prepass 已经修改源码
- 但 `build_gate` 失败
- 优先处理 `build_gate.suspect_files`

### `ready_for_clean_refresh`

- recheck 后无 residual
- 可进入 `clean_refresh`

### `done`

- 批次已完成并关闭

## 8. 排障建议

### prepass 看起来改了不该改的 checks

优先检查：

- `tools/toolchain/services/clang_tidy_runner.py`
- `tools/toolchain/commands/tidy_fix.py`
- `tools/toolchain/config/workflow.toml`
- 仓库根 `.clang-tidy`

### 状态和日志不一致

优先检查：

- `tools/toolchain/services/tidy_runtime.py`
- `tools/toolchain/commands/tidy_batch.py`
- `tools/toolchain/commands/tidy_recheck.py`

### residual 分类不合理

优先检查：

- `tools/toolchain/services/tidy_residuals.py`
- `tools/toolchain/services/fix_strategy.py`

### 批次编号看不懂

优先检查：

- `python tools/run.py tidy-status`
- `tools/toolchain/services/tidy_runtime.py`

## 9. 相关文档

- 代码地图：`docs/toolchain_tidy_automation.md`
- 文档索引：`docs/README.md`
