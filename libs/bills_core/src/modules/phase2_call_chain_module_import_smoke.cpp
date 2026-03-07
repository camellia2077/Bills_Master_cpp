// modules/phase2_call_chain_module_import_smoke.cpp
import bill.core.abi;
import bill.core.application.workflow_use_case;
import bill.core.reports.standard_report_assembler;
import bill.core.reports.standard_report_json_serializer;

namespace {
namespace abi = bills::core::modules::abi;

using bills::core::modules::application::WorkflowUseCase;
using bills::core::modules::reports::MonthlyReportData;
using bills::core::modules::reports::StandardReportAssembler;
using bills::core::modules::reports::StandardReportJsonSerializer;
using bills::core::modules::reports::YearlyReportData;

[[maybe_unused]] auto kWorkflowValidate = &WorkflowUseCase::Validate;
[[maybe_unused]] auto kWorkflowIngest = &WorkflowUseCase::Ingest;

[[maybe_unused]] auto kAssemblerFromMonthly = &StandardReportAssembler::FromMonthly;
[[maybe_unused]] auto kAssemblerFromYearly = &StandardReportAssembler::FromYearly;
[[maybe_unused]] auto kReportToJson = &StandardReportJsonSerializer::ToJson;
[[maybe_unused]] auto kReportToString = &StandardReportJsonSerializer::ToString;

[[maybe_unused]] auto kHandleValidate = &abi::handle_validate_command;
[[maybe_unused]] auto kHandleQuery = &abi::handle_query_command;
[[maybe_unused]] abi::Json kRequestTemplate = abi::Json::object();
[[maybe_unused]] MonthlyReportData kMonthlyReportData{};
[[maybe_unused]] YearlyReportData kYearlyReportData{};
}  // namespace
