## 下一步重构方向（详细）

目标：先稳定层级命名（application/use_cases、ports、adapters），在此基础上逐步拆 presentation 与 reports。

### 1) 稳定层级命名（优先级最高）
1. 确认目录规范并固定约定
   - application: 只放用例与应用层编排（use_cases）。
   - ports: 统一放在 src/ports（作为应用层输出端口）。
   - adapters: 统一放在 src/adapters（IO/DB/JSON/Config）。
2. 用例目录统一
   - 当前已将 WorkflowUseCase 迁移到 application/use_cases。
   - 后续新增用例都放 application/use_cases，避免继续落到 app_controller。
3. CMake 与 include 校准
   - 新增/迁移任何文件时，更新 `cmake/source_files.cmake`。
   - include 仍保持从 src 根的绝对路径（如 `application/use_cases/...`）。

### 2) presentation 层拆分（中优先级）
目标：控制器只做 CLI/参数解析/调用用例，避免业务逻辑。
1. 建议目录
   - presentation/commands（命令解析）
   - presentation/controllers（WorkflowController、ExportController）
2. 迁移建议
   - `app_controller/workflow` → `presentation/controllers`
   - `command_handler` → `presentation/commands`
3. 迁移步骤
   - 先移动目录，再全局替换 include。
   - 更新 CMake。
   - 确保 controller 只调用 use_cases，不包含业务流程。

### 3) 统一错误模型扩展到入口层（中优先级）
目标：从 CLI 到用例统一 Result/Error。
1. 改造范围
   - `AppController`、`command_handler` 返回 `Status/Result`，避免 bool + std::cerr。
2. 迁移顺序
   - 先改 `WorkflowController` 的调用方（AppController）。
   - 再改命令层（CommandFacade / 各 Command）。
3. 输出规范
   - 统一在 presentation 层打印错误。
   - 业务层只返回 Error，不直接打印。

### 4) 配置端口进一步归位（中优先级）
目标：配置校验/解析彻底视为适配器。
1. 目录建议
   - `config_validator` 可并入 `adapters/config`（若仅负责 JSON 校验）。
2. 迁移步骤
   - 合并 `config_validator` 到 adapters/config。
   - 仅暴露 `ConfigProvider` 给应用层。

### 5) reports 拆分（次优先级）
目标：查询与格式化解耦。
1. 端口设计
   - QueryPort（读取/聚合）
   - FormatterPort（格式化输出）
2. 迁移步骤
   - 把 queries 迁到 application/report。
   - formatters 保留在 adapters/report_formatters。

### 6) 最后清理命名与冗余（低优先级）
1. 统一文件命名（Converter/Processor/Transformer）
2. 删除未使用文件（旧 ConfigLoader/冗余 parser）
3. 清理历史 include 路径

---

### 建议执行顺序（避免大改导致编译失败）
1. 完成 presentation 迁移（含 CMake/include 修复）
2. 入口层统一错误模型
3. 配置验证归并到 adapters/config
4. reports 拆分
5. 清理命名与冗余

### 执行前检查清单
- CMake 里所有源文件路径更新完毕
- include 仍使用 src 根绝对路径
- controller 只负责装配与调用 use_cases
- ports/adapters 不出现反向依赖
