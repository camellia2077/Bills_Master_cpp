// modules/phase3_layer_module_import_smoke.cpp
import bill.core.application.workflow_use_case;
import bill.core.billing.pipeline;
import bill.core.reports.standard_report_assembler;
import bill.core.reports.standard_report_json_serializer;
import bill.core.reports.standard_report_renderer_registry;

namespace {
using bills::core::modules::application::WorkflowUseCase;
using bills::core::modules::billing::BillProcessingPipeline;
using bills::core::modules::reports::StandardReportAssembler;
using bills::core::modules::reports::StandardReportJsonSerializer;
using bills::core::modules::reports::StandardReportRendererRegistry;

[[maybe_unused]] auto kWorkflowValidate = &WorkflowUseCase::Validate;
[[maybe_unused]] auto kWorkflowConvert = &WorkflowUseCase::Convert;
[[maybe_unused]] auto kPipelineValidate = &BillProcessingPipeline::validate_content;
[[maybe_unused]] auto kPipelineConvert = &BillProcessingPipeline::convert_content;
[[maybe_unused]] auto kAssemblerFromMonthly = &StandardReportAssembler::FromMonthly;
[[maybe_unused]] auto kAssemblerFromYearly = &StandardReportAssembler::FromYearly;
[[maybe_unused]] auto kReportToJson = &StandardReportJsonSerializer::ToJson;
[[maybe_unused]] auto kReportToString = &StandardReportJsonSerializer::ToString;
[[maybe_unused]] auto kListFormats = &StandardReportRendererRegistry::ListAvailableFormats;
[[maybe_unused]] auto kNormalizeFormat = &StandardReportRendererRegistry::NormalizeFormat;
[[maybe_unused]] auto kIsFormatAvailable =
    &StandardReportRendererRegistry::IsFormatAvailable;
[[maybe_unused]] auto kRenderReport = &StandardReportRendererRegistry::Render;
}  // namespace
