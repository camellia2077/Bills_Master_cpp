import bill.core.application.workflow_use_case;
import bill.core.billing.pipeline;
import bill.core.reports.standard_report_assembler;
import bill.core.reports.standard_report_json_serializer;

namespace {
using bills::core::modules::application::WorkflowUseCase;
using bills::core::modules::billing::BillProcessingPipeline;
using bills::core::modules::reports::StandardReportAssembler;
using bills::core::modules::reports::StandardReportJsonSerializer;

[[maybe_unused]] auto kWorkflowValidate = &WorkflowUseCase::Validate;
[[maybe_unused]] auto kWorkflowConvert = &WorkflowUseCase::Convert;
[[maybe_unused]] auto kPipelineValidate = &BillProcessingPipeline::validate_content;
[[maybe_unused]] auto kPipelineConvert = &BillProcessingPipeline::convert_content;
[[maybe_unused]] auto kAssemblerFromMonthly = &StandardReportAssembler::FromMonthly;
[[maybe_unused]] auto kAssemblerFromYearly = &StandardReportAssembler::FromYearly;
[[maybe_unused]] auto kReportToJson = &StandardReportJsonSerializer::ToJson;
[[maybe_unused]] auto kReportToString = &StandardReportJsonSerializer::ToString;
}  // namespace
