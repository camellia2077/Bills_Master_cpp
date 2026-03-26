import bill.core.ingest.bill_workflow_service;
import bill.core.ingest.bill_processing_pipeline;
import bill.core.ingest.bill_json_serializer;
import bill.core.query.query_service;
import bill.core.reporting.render_service;
import bill.core.reporting.renderer_registry;
import bill.core.reporting.standard_report_assembler;
import bill.core.reporting.standard_report_json_serializer;
import bill.core.record_template.service;

namespace {
using bills::core::modules::ingest::BillJsonSerializer;
using bills::core::modules::ingest::BillProcessingPipeline;
using bills::core::modules::ingest::BillWorkflowService;
using bills::core::modules::query::QueryService;
using bills::core::modules::reporting::ReportRenderService;
using bills::core::modules::reporting::StandardReportAssembler;
using bills::core::modules::reporting::StandardReportJsonSerializer;
using bills::core::modules::reporting::StandardReportRendererRegistry;
using bills::core::modules::record_template::RecordTemplateService;

[[maybe_unused]] auto kSerializeEntry = &BillJsonSerializer::serialize;
[[maybe_unused]] auto kWorkflowValidateEntry = &BillWorkflowService::Validate;
[[maybe_unused]] auto kPipelineValidateEntry = &BillProcessingPipeline::validate_content;
[[maybe_unused]] auto kQueryYearEntry = &QueryService::QueryYear;
[[maybe_unused]] auto kRenderEntry = &ReportRenderService::Render;
[[maybe_unused]] auto kAssemblerEntry = &StandardReportAssembler::FromYearly;
[[maybe_unused]] auto kStandardJsonEntry = &StandardReportJsonSerializer::ToString;
[[maybe_unused]] auto kRegistryEntry = &StandardReportRendererRegistry::ListAvailableFormats;
[[maybe_unused]] auto kRecordTemplateEntry = &RecordTemplateService::GenerateTemplates;
}  // namespace
