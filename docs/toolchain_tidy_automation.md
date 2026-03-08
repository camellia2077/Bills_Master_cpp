# bills_tracer Clang-Tidy Automation Code Map

本文档面向后续维护 toolchain 的 agent / 开发者，目标是回答两个问题：

1. `tools/` 下面这么多 Python 文件，改某个行为应该先看哪里？
2. 当前 clang-tidy 批处理自动化是如何分层的？

如果你只关心“操作顺序”，请看：

- `docs/toolchain_sop.md`

## 1. 先记住两个入口

### 用户入口

- `python tools/run.py ...`

这是仓库内统一的 Python toolchain 入口。

### 历史/外围入口

- `python tools/verify/verify.py ...`
- `tools/flows/*.py`

这些仍然存在，但对 clang-tidy SOP 来说，推荐把 `tools/run.py` 视为主入口，`verify.py` 视为底层构建/测试入口。

## 2. 顶层目录怎么分工

### `tools/toolchain/`

新的统一 Python toolchain 实现，clang-tidy 自动化的核心都在这里。

### `tools/verify/`

统一 dist/验证入口。`tidy-batch` 里的 baseline verify / dist gate 最终都会调用这里。

### `tools/flows/`

项目已有的 dist/test flow 脚本。对 tidy SOP 来说，它们更多是被 `verify` 或其他命令间接调用。

### `temp/tidy/`

运行时产物目录，不是源码目录。包括：

- 原始日志
- split 后的任务
- canonical state
- projection JSON
- recheck diagnostics

## 3. `tools/toolchain/` 内部结构

### `tools/toolchain/cli/`

负责 CLI 装配，不放业务逻辑。

- `main.py`
  - 真正的 CLI 入口
  - 创建 `Context`
  - 解析参数
  - 分发到对应 command
- `parser.py`
  - 所有 `python tools/run.py <subcommand>` 的 argparse 定义
  - 新增命令或参数时先看这里

### `tools/toolchain/commands/`

按命令组织 orchestration。

这里的文件应该尽量薄：负责读参数、调用 service、写用户可见输出，不要堆太多底层逻辑。

最重要的几个文件：

- `tidy.py`
  - 触发完整 `clang-tidy` 构建并捕获原始日志
- `tidy_split.py`
  - 把原始日志切成任务
- `tidy_batch.py`
  - 当前 SOP 总控
  - 如果要改 phase 顺序、失败分支、dist gate 行为，先看这里
- `tidy_fix.py`
  - safe-fix / 手动 fix pass 的命令入口
- `tidy_recheck.py`
  - targeted recheck 的命令入口
- `tidy_status.py`
  - 全局/单批状态查看
- `tidy_show.py`
  - manifest + batch phase 的只读视图
- `tidy_refresh.py`
  - 任务刷新逻辑
- `clean.py`
  - 清理已完成任务 / 批次
- `verify.py`
  - 转发到 `tools/verify/verify.py`

### `tools/toolchain/services/`

真正的底层逻辑层。大部分“为什么会这样”的答案都在这里。

#### 与 clang-tidy 执行直接相关

- `clang_tidy_runner.py`
  - 最底层 `clang-tidy` 调用器
  - 负责组命令、落日志、处理 `-fix`
  - 如果怀疑 checks 过滤没收住，先看这里

- `format_runner.py`
  - `clang-format` 执行器

#### 与 SOP / 状态模型直接相关

- `tidy_runtime.py`
  - canonical state 的核心
  - 负责：
    - `latest.json`
    - `batches/<batch_id>.json`
    - phase state 更新
    - `numbering_context`
    - legacy projection 生成
  - 如果状态不对、批次编号解释不对，先看这里

- `tidy_residuals.py`
  - residual diagnostics 分类器
  - 负责：
    - `preferred_action`
    - `fallback_action`
    - `reason_template`
    - `unexpected_fixable_count`

- `fix_strategy.py`
  - 检查一个 batch 里的 checks 更偏向：
    - `safe_refactor`
    - `manual_only`
  - 也负责补充：
    - `safe_fix_checks_present`
    - `suppression_candidates_present`

#### 与任务队列相关

- `tidy_queue.py`
  - 读取 / 生成 manifest
  - 维护 task / batch 级视图
  - 如果 `tidy-show` 里列的任务不对，先看这里

- `task_sorter.py`
  - 任务排序逻辑

- `incremental_mapper.py`
  - refresh 时的增量映射辅助

#### 与日志/路径相关

- `tidy_log_parser.py`
  - 从 clang-tidy 输出中抽取 diagnostics
  - 如果 log 有 warning，但 JSONL 没收进去，先看这里

- `tidy_paths.py`
  - 所有 `temp/tidy/*` 相关路径集中定义
  - 如果目录位置改了，先看这里

- `tidy_state.py`
  - 轻量 JSON state 读写助手

- `tidy_cleaner.py`
  - clean 相关辅助

- `timestamps.py`
  - 时间戳辅助

### `tools/toolchain/core/`

跨命令基础设施。

- `context.py`
  - 仓库上下文、配置、Python 路径等
- `config.py`
  - `workflow.toml` 配置模型
- `process_runner.py`
  - 子进程执行辅助
- `path_display.py`
  - CLI 输出中的路径展示格式

### `tools/toolchain/config/`

- `workflow.toml`
  - toolchain 的主要配置入口
  - 当前 tidy 相关重点配置包括：
    - scope
    - safe-fix whitelist
    - suppression policy
    - status display

### `tools/toolchain/scripts/`

放小脚本或局部辅助脚本。不是 clang-tidy SOP 的主控层。

## 4. 现在这套 clang-tidy 自动化是怎么分层的

可以把它理解成 5 层：

### 第 1 层：CLI 分发

- `tools/run.py`
- `tools/toolchain/cli/main.py`
- `tools/toolchain/cli/parser.py`

### 第 2 层：命令 orchestration

- `tools/toolchain/commands/*.py`

这一层定义“做什么顺序调用什么”。

### 第 3 层：领域 service

- `tools/toolchain/services/*.py`

这一层定义“怎么做”。

### 第 4 层：底层执行器

- `clang_tidy_runner.py`
- `format_runner.py`
- `verify.py -> tools/verify/verify.py`

### 第 5 层：运行时产物

- `temp/tidy/`

## 5. 常见改动该去哪里改

### 新增一个 `python tools/run.py tidy-xxx` 命令

先看：

- `tools/toolchain/cli/parser.py`
- `tools/toolchain/commands/`

### 改 `tidy-batch --preset sop` 的阶段顺序

先看：

- `tools/toolchain/commands/tidy_batch.py`
- `tools/toolchain/services/tidy_runtime.py`

### 改 safe-fix 白名单

先看：

- `tools/toolchain/config/workflow.toml`
- `tools/toolchain/core/config.py`
- `tools/toolchain/services/clang_tidy_runner.py`
- `tools/toolchain/commands/tidy_fix.py`

### 改 residual 的“重构 vs suppression”建议

先看：

- `tools/toolchain/services/tidy_residuals.py`
- `tools/toolchain/services/fix_strategy.py`

### 改 batch 编号、closed ranges 的解释

先看：

- `tools/toolchain/services/tidy_runtime.py`
- `tools/toolchain/commands/tidy_status.py`
- `tools/toolchain/commands/tidy_show.py`

### 改 task 切分或 log 解析

先看：

- `tools/toolchain/commands/tidy_split.py`
- `tools/toolchain/commands/tidy_support.py`
- `tools/toolchain/services/tidy_log_parser.py`
- `tools/toolchain/services/tidy_queue.py`

## 6. 当前与旧设计相比的关键变化

### 从“多个松散 JSON”改为“canonical state + projection”

新的事实来源：

- `temp/tidy/state/latest.json`
- `temp/tidy/state/batches/<batch_id>.json`

旧文件继续生成，但只是 projection：

- `temp/tidy/tidy_result.json`
- `temp/tidy/batch_status.json`
- `temp/tidy/tidy_batch_checkpoint.json`

### 从“agent 手工串步骤”改为“Python 驱动 SOP”

现在不推荐 agent 手工完成：

- baseline verify
- tidy-fix
- targeted recheck
- residual 摘取
- batch 编号解释

### 从“看原始日志”改为“优先读状态命令”

推荐优先级：

1. `tidy-status`
2. batch state JSON
3. `tidy-show`
4. 原始 `.log` / `.jsonl`

## 7. 当前最值得记住的源码入口

如果只能记住 8 个文件，记这几个：

- `tools/toolchain/cli/parser.py`
- `tools/toolchain/commands/tidy_batch.py`
- `tools/toolchain/commands/tidy_recheck.py`
- `tools/toolchain/commands/tidy_status.py`
- `tools/toolchain/services/clang_tidy_runner.py`
- `tools/toolchain/services/tidy_runtime.py`
- `tools/toolchain/services/tidy_residuals.py`
- `tools/toolchain/config/workflow.toml`

## 8. 相关文档

- SOP：`docs/toolchain_sop.md`
- 文档索引：`docs/README.md`
