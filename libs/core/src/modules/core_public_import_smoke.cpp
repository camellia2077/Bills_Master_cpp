import bill.core.common.process_stats;
import bill.core.common.result;
import bill.core.common.text_normalizer;
import bill.core.common.version;
import bill.core.domain.bill_record;
import bill.core.config.document_types;
import bill.core.config.bundle_service;
import bill.core.ports.bills_repository;
import bill.core.ports.report_data_gateway;
import bill.core.record_template.types;

namespace {
using bills::core::modules::common_process_stats::ProcessStats;
using bills::core::modules::common_result::Error;
using bills::core::modules::config::Config;
using bills::core::modules::config::ConfigBundleService;
using bills::core::modules::config::ConfigDocumentBundle;
using bills::core::modules::config::RuntimeConfigBundle;
using bills::core::modules::domain_bill_record::ParsedBill;
using bills::core::modules::ports::BillRepository;
using bills::core::modules::ports::ReportDataGateway;
using bills::core::modules::record_template::TemplateGenerationRequest;

[[maybe_unused]] ProcessStats kProcessStats{};
[[maybe_unused]] Error kError{};
[[maybe_unused]] ConfigDocumentBundle kDocuments{};
[[maybe_unused]] RuntimeConfigBundle kConfigBundle{};
[[maybe_unused]] ParsedBill kParsedBill{};
[[maybe_unused]] TemplateGenerationRequest kTemplateRequest{};
[[maybe_unused]] auto* kConfigServiceEntry = &ConfigBundleService::Validate;
[[maybe_unused]] auto kRepositoryEntry = &BillRepository::InsertBill;
[[maybe_unused]] auto kGatewayMonthEntry = &ReportDataGateway::ReadMonthlyData;
[[maybe_unused]] constexpr auto kVersion =
    bills::core::modules::common_version::kVersion;
}  // namespace
