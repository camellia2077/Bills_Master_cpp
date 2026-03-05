# Struct First 重构：Phase 0/1 执行产物

## 1. 范围与目标
- 覆盖计划文件：`temp/md-json-render.md` 的 Phase 0 与 Phase 1。
- 目标：在不改动业务行为的前提下，先完成“基线冻结 + 现状差异盘点 + 契约边界收敛”。

## 2. Phase 0：基线冻结与差异盘点

### 2.1 基线快照（冻结结果）
- 当前冻结基线目录：`tests/baseline/report_snapshots/`
- 当前 manifest：`tests/baseline/report_snapshots/manifest.json`
- 当前已冻结格式：`md`、`json`
- 当前冻结场景：
  - monthly：`2024-01`
  - yearly：`2024`
  - range：`2024-03`、`2024-04`
- 冻结/校验脚本：
  - 冻结：`python tools/verify/freeze_report_snapshots.py`
  - 校验：`python tools/verify/check_report_snapshots.py`

### 2.2 三条导出管线调用图（现状）
```text
legacy/model-first/json-first
  -> ReportExportService::export_*()
    -> ReportDataGateway::Read*Data()
    -> StandardReportAssembler::From*(...)            // 组装 StandardReport
    -> StandardReportJsonSerializer::ToString(...)    // 产出标准 JSON
    -> ReportExporter::export_*_standard_json(...)    // 保存标准 JSON
    -> 分支:
       A) 标准渲染分支:
          - md: 所有 pipeline 都走标准渲染
          - tex: 仅 model-first/json-first 走标准渲染
       B) legacy 插件分支:
          - tex/rst 等走 formatter->format_report(*ReportData)
```

### 2.3 语义差异清单（现状）
1. **yearly 月结余公式存在分叉**
   - `StandardReportAssembler` 写入 `items.monthly_summary[].balance` 时使用 `income - expense`。
   - 多个渲染器/插件输出 yearly 月结余时又按 `income + expense` 重算。
2. **渲染层对 JSON 字段未完全“按值消费”**
   - yearly 渲染路径会重算月结余，而不是直接消费 `items.monthly_summary[].balance`。
3. **管线能力不完全对称**
   - `md` 已统一进入标准渲染链。
   - `tex` 在 `legacy` 仍可绕开标准渲染链。
4. **基线覆盖存在缺口**
   - 快照基线目前仅冻结 `md/json`；`tex` 依赖一致性门禁项目对比，尚未纳入 `report_snapshots/manifest.json`。

### 2.4 结论
- 现有代码已具备“单次组装 StandardReport + 标准 JSON 落盘”的主干能力。
- 真正的单一数据源仍未闭环（renderers 与 legacy 分叉尚存），Phase 2/3 继续收敛。

## 3. Phase 1：契约收敛（StandardReport 作为唯一事实源）

### 3.1 契约所有权（Owner）
- 结构体事实源：`libs/bills_core/src/reports/standard_json/standard_report_dto.hpp`
- 组装层：`libs/bills_core/src/reports/standard_json/standard_report_assembler.cpp`
- JSON 渲染层：`libs/bills_core/src/reports/standard_json/standard_report_json_serializer.cpp`
- 导出编排层：`libs/bills_core/src/reports/core/report_export_service.cpp`

### 3.2 分层边界（Phase 1 决议）
- **业务聚合层**：仅负责 `*ReportData -> StandardReport`，不做格式细节。
- **渲染层**：负责 `StandardReport -> format(json/md/tex)`。
- **导出编排层**：负责 pipeline 路由、文件落盘、插件回退策略。

### 3.3 版本策略（schema_version）
- `1.x`：仅允许可选字段增量，不破坏既有字段语义。
- 语义变更/字段删除：必须升 `2.0.0`。
- JSON 视为 StandardReport 的一种渲染输出，而非业务事实源本体。

### 3.4 阶段性约束
- 在 Phase 2 完成前，允许存在“字符串 JSON 渲染入口 + 兼容桥接”。
- Phase 2 目标是让 `md/tex` 直接消费 `StandardReport`，移除 JSON 中转依赖。

## 4. 进入 Phase 2 的前置条件（已满足）
- [x] 现状调用图已落文档。
- [x] 差异清单已明确。
- [x] 契约所有权与边界已明确。
- [x] `standard_report_json_schema_v1.md` 已补充“JSON 渲染层定位”说明。
