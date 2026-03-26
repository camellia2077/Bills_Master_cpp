#ifndef BILLS_IO_HOST_FLOW_SUPPORT_HPP_
#define BILLS_IO_HOST_FLOW_SUPPORT_HPP_

#include <filesystem>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include "config/config_bundle_service.hpp"
#include "ingest/bill_workflow_service.hpp"
#include "query/query_service.hpp"
#include "record_template/import_preflight_service.hpp"
#include "record_template/record_template_types.hpp"
#include "reporting/standard_report/standard_report_dto.hpp"

class YearPartitionOutputPathBuilder;

namespace bills::io {

struct HostConfigContext {
  ConfigDocumentBundle documents;
  ValidatedConfigBundle validated;
};

struct ParseBundleExportResult {
  std::size_t exported_record_files = 0U;
  std::size_t exported_config_files = 0U;
};

struct ParseBundleImportResult {
  bool ok = true;
  std::string message;
  std::string failed_phase;
  std::size_t imported_record_files = 0U;
  std::size_t imported_config_files = 0U;
  std::size_t imported_bills = 0U;
  ConfigBundleValidationReport config_validation;
  BillWorkflowBatchResult record_validation;
  BillWorkflowBatchResult db_ingest;
};

struct HostDatabaseIngestResult {
  bool database_reset = false;
  BillWorkflowBatchResult ingest;
};

struct HostRecordDirectoryImportResult {
  std::size_t processed = 0U;
  std::size_t imported = 0U;
  std::size_t overwritten = 0U;
  std::size_t failure = 0U;
  std::size_t invalid = 0U;
  std::size_t duplicate_period_conflicts = 0U;
  std::string first_failure_message;
};

struct HostRecordDatabaseSyncResult {
  bool period_matches = true;
  std::string actual_period;
  BillWorkflowBatchResult ingest;
};

struct HostConfigInspectionResult {
  ConfigInspectResult inspect;
  std::vector<std::string> enabled_export_formats;
  std::vector<std::string> available_export_formats;
};

struct HostTemplateGenerationRequest {
  std::string period;
  std::string start_period;
  std::string end_period;
  std::string start_year;
  std::string end_year;
};

struct HostQueryResult {
  QueryExecutionResult execution;
  StandardReport standard_report;
  std::string report_markdown;
  std::string standard_report_json;
  std::size_t matched_bills = 0U;
  std::size_t transaction_count = 0U;
};

enum class HostReportExportScope {
  kYear,
  kMonth,
  kRange,
  kAllMonths,
  kAllYears,
  kAll,
};

struct HostReportExportRequest {
  HostReportExportScope scope = HostReportExportScope::kMonth;
  std::string primary_value;
  std::string secondary_value;
  std::vector<std::string> formats;
  std::filesystem::path db_path;
  std::filesystem::path export_dir;
  std::map<std::string, std::string> format_folder_names;
};

struct HostReportExportResult {
  bool ok = true;
  std::vector<std::string> attempted_formats;
  std::vector<std::string> failed_formats;
  std::size_t exported_count = 0U;
  std::filesystem::path export_dir;
};

[[nodiscard]] auto LoadValidatedConfigContext(
    const std::filesystem::path& config_dir) -> Result<HostConfigContext>;

[[nodiscard]] auto LoadSourceDocuments(const std::filesystem::path& root_path,
                                       std::string_view extension)
    -> Result<SourceDocumentBatch>;

[[nodiscard]] auto ListEnabledExportFormats(
    const std::filesystem::path& config_dir) -> Result<std::vector<std::string>>;

[[nodiscard]] auto InspectConfig(const std::filesystem::path& config_dir)
    -> Result<HostConfigInspectionResult>;

[[nodiscard]] auto GenerateTemplatesFromConfig(
    const std::filesystem::path& config_dir,
    const HostTemplateGenerationRequest& request)
    -> Result<TemplateGenerationResult>;

[[nodiscard]] auto PreviewRecordDocuments(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir) -> Result<RecordPreviewResult>;

[[nodiscard]] auto ListRecordPeriods(const std::filesystem::path& input_path)
    -> Result<ListedPeriodsResult>;

[[nodiscard]] auto ValidateDocuments(const std::filesystem::path& input_path,
                                     const std::filesystem::path& config_dir)
    -> Result<BillWorkflowBatchResult>;

[[nodiscard]] auto ConvertDocuments(const std::filesystem::path& input_path,
                                    const std::filesystem::path& config_dir,
                                    bool include_serialized_json = false)
    -> Result<BillWorkflowBatchResult>;

[[nodiscard]] auto IngestDocuments(const std::filesystem::path& input_path,
                                   const std::filesystem::path& config_dir,
                                   const std::filesystem::path& db_path,
                                   bool include_serialized_json = false)
    -> Result<BillWorkflowBatchResult>;

[[nodiscard]] auto IngestDocumentsToDatabase(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir,
    const std::filesystem::path& db_path,
    bool reset_legacy_database = false,
    bool include_serialized_json = false) -> Result<HostDatabaseIngestResult>;

[[nodiscard]] auto ImportJsonDocuments(const std::filesystem::path& input_path,
                                       const std::filesystem::path& db_path)
    -> Result<BillWorkflowBatchResult>;

[[nodiscard]] auto ImportRecordDirectoryToWorkspace(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir,
    const std::filesystem::path& records_root)
    -> Result<HostRecordDirectoryImportResult>;

[[nodiscard]] auto ExtractSingleRecordPeriod(
    const std::filesystem::path& input_path) -> Result<std::string>;

[[nodiscard]] auto SyncSingleRecordToDatabase(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir,
    const std::filesystem::path& db_path, std::string_view expected_period,
    bool include_serialized_json = false) -> Result<HostRecordDatabaseSyncResult>;

[[nodiscard]] auto PreflightImportDocuments(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir,
    const std::vector<std::string>& existing_workspace_periods = {},
    const std::vector<std::string>& existing_db_periods = {})
    -> Result<ImportPreflightResult>;

[[nodiscard]] auto QueryYearReport(const std::filesystem::path& db_path,
                                   std::string_view iso_year)
    -> Result<HostQueryResult>;

[[nodiscard]] auto QueryMonthReport(const std::filesystem::path& db_path,
                                    std::string_view iso_month)
    -> Result<HostQueryResult>;

[[nodiscard]] auto ListAvailableMonths(const std::filesystem::path& db_path)
    -> Result<std::vector<std::string>>;

[[nodiscard]] auto RenderQueryReport(const HostQueryResult& query_result,
                                     std::string_view format_name)
    -> Result<std::string>;

[[nodiscard]] auto ExportReports(const HostReportExportRequest& request)
    -> Result<HostReportExportResult>;

[[nodiscard]] auto ExportParseBundle(const std::filesystem::path& records_root,
                                     const std::filesystem::path& config_dir,
                                     const std::filesystem::path& output_zip)
    -> Result<ParseBundleExportResult>;

[[nodiscard]] auto ImportParseBundle(const std::filesystem::path& bundle_zip,
                                     const std::filesystem::path& config_dir,
                                     const std::filesystem::path& records_root,
                                     std::optional<std::filesystem::path> db_path = std::nullopt)
    -> Result<ParseBundleImportResult>;

[[nodiscard]] auto WriteTemplateFiles(
    const std::filesystem::path& output_dir,
    const TemplateGenerationResult& result) -> Result<std::vector<std::string>>;

[[nodiscard]] auto WriteSerializedJsonOutputs(
    const std::vector<BillWorkflowFileResult>& files,
    const YearPartitionOutputPathBuilder& output_path_builder)
    -> Result<std::vector<std::string>>;

}  // namespace bills::io

#endif  // BILLS_IO_HOST_FLOW_SUPPORT_HPP_
