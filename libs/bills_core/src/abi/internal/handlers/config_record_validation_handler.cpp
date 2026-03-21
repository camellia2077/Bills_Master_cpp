#include <exception>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "config_loading/config_bundle_validation.hpp"
#include "record_template/import_preflight_service.hpp"
#include "record_template/record_template_service.hpp"

#if BILLS_CORE_MODULES_ENABLED
import bill.core.abi;
namespace core_abi = bills::core::modules::abi;
#else
#include "abi/internal/abi_shared.hpp"
namespace core_abi = bills::core::abi;
#endif

namespace bills::core::abi {

#if BILLS_CORE_MODULES_ENABLED
using Json = core_abi::Json;
#endif

namespace {

auto MakeServiceErrorResponse(const RecordTemplateError& error,
                              std::string_view default_message) -> std::string {
  const char* error_code = core_abi::error_code::kSystemNotImplemented;
  if (error.category == RecordTemplateErrorCategory::kRequest) {
    error_code = core_abi::error_code::kParamInvalidRequest;
  } else if (error.category == RecordTemplateErrorCategory::kConfig) {
    error_code = core_abi::error_code::kParamInvalidConfig;
  } else if (error.category == RecordTemplateErrorCategory::kInputPath) {
    error_code = core_abi::error_code::kParamInvalidInputPath;
  } else if (error.category == RecordTemplateErrorCategory::kOutputPath) {
    error_code = core_abi::error_code::kParamInvalidOutputPath;
  }

  Json data;
  data["detail"] = FormatRecordTemplateError(error);
  return core_abi::make_response(false, error_code, std::string(default_message),
                                 std::move(data));
}

auto SerializeNullableString(const std::string& value) -> Json {
  if (value.empty()) {
    return nullptr;
  }
  return value;
}

auto SerializeNullablePath(const std::filesystem::path& value) -> Json {
  if (value.empty()) {
    return nullptr;
  }
  return value.string();
}

auto SerializeNullableInt(int value) -> Json {
  if (value <= 0) {
    return nullptr;
  }
  return value;
}

auto SerializeValidationIssue(const ValidationIssue& issue) -> Json {
  Json item;
  item["source_kind"] = issue.source_kind;
  item["stage"] = issue.stage;
  item["code"] = issue.code;
  item["message"] = issue.message;
  item["path"] = SerializeNullablePath(issue.path);
  item["line"] = SerializeNullableInt(issue.line);
  item["column"] = SerializeNullableInt(issue.column);
  item["field_path"] = SerializeNullableString(issue.field_path);
  item["severity"] = issue.severity;
  return item;
}

auto IssueIdentity(const ValidationIssue& issue) -> std::string {
  return issue.source_kind + "|" + issue.stage + "|" + issue.code + "|" +
         issue.message + "|" + issue.path.string() + "|" +
         std::to_string(issue.line) + "|" + std::to_string(issue.column) + "|" +
         issue.field_path + "|" + issue.severity;
}

auto SerializeValidationIssues(const std::vector<ValidationIssue>& issues) -> Json {
  Json items = Json::array();
  for (const auto& issue : issues) {
    items.push_back(SerializeValidationIssue(issue));
  }
  return items;
}

auto CollectConfigIssues(const ConfigBundleValidationReport& report)
    -> std::vector<ValidationIssue> {
  std::vector<ValidationIssue> issues;
  for (const auto& file : report.files) {
    issues.insert(issues.end(), file.issues.begin(), file.issues.end());
  }
  return issues;
}

auto CollectRecordIssues(const RecordPreviewResult& result)
    -> std::vector<ValidationIssue> {
  std::vector<ValidationIssue> issues;
  for (const auto& file : result.files) {
    issues.insert(issues.end(), file.issues.begin(), file.issues.end());
  }
  return issues;
}

auto CollectPreflightIssues(const ImportPreflightResult& result)
    -> std::vector<ValidationIssue> {
  std::vector<ValidationIssue> issues;
  std::set<std::string> seen;

  auto append_unique = [&issues, &seen](const ValidationIssue& issue) {
    if (seen.insert(IssueIdentity(issue)).second) {
      issues.push_back(issue);
    }
  };

  for (const auto& issue : CollectConfigIssues(result.config_validation)) {
    append_unique(issue);
  }
  for (const auto& issue : CollectRecordIssues(result.record_validation)) {
    append_unique(issue);
  }
  for (const auto& issue : result.issues) {
    append_unique(issue);
  }

  return issues;
}

auto SerializeConfigFileValidationReport(const ConfigFileValidationReport& report)
    -> Json {
  Json item;
  item["source_kind"] = report.source_kind;
  item["file_name"] = report.file_name;
  item["path"] = SerializeNullablePath(report.path);
  item["ok"] = report.ok;
  item["issues"] = SerializeValidationIssues(report.issues);
  return item;
}

auto SerializeConfigBundleValidationReport(const ConfigBundleValidationReport& report)
    -> Json {
  Json files = Json::array();
  for (const auto& file : report.files) {
    files.push_back(SerializeConfigFileValidationReport(file));
  }

  Json data;
  data["processed"] = report.processed;
  data["success"] = report.success;
  data["failure"] = report.failure;
  data["all_valid"] = report.ok;
  data["files"] = std::move(files);
  data["issues"] = SerializeValidationIssues(CollectConfigIssues(report));
  data["enabled_export_formats"] = report.enabled_export_formats;
  data["available_export_formats"] = report.available_export_formats;
  return data;
}

auto SerializeRecordPreviewFile(const RecordPreviewFile& file) -> Json {
  Json item;
  item["path"] = file.path.string();
  item["ok"] = file.ok;
  item["file_name_period"] = SerializeNullableString(file.file_name_period);
  item["file_name_matches_period"] = file.file_name_matches_period;
  item["period"] = SerializeNullableString(file.period);
  item["year"] = SerializeNullableInt(file.year);
  item["month"] = SerializeNullableInt(file.month);
  item["transaction_count"] = file.transaction_count;
  item["total_income"] = file.total_income;
  item["total_expense"] = file.total_expense;
  item["balance"] = file.balance;
  item["error"] = SerializeNullableString(file.error);
  item["issues"] = SerializeValidationIssues(file.issues);
  return item;
}

auto SerializeRecordPreviewResult(const RecordPreviewResult& result) -> Json {
  Json files = Json::array();
  for (const auto& file : result.files) {
    files.push_back(SerializeRecordPreviewFile(file));
  }

  Json data;
  data["input_path"] = result.input_path.string();
  data["processed"] = result.processed;
  data["success"] = result.success;
  data["failure"] = result.failure;
  data["all_valid"] = result.failure == 0U;
  data["periods"] = result.periods;
  data["files"] = std::move(files);
  data["issues"] = SerializeValidationIssues(CollectRecordIssues(result));
  return data;
}

auto SerializeImportPreflightResult(const ImportPreflightResult& result) -> Json {
  Json data;
  data["all_clear"] = result.all_clear;
  data["processed"] = result.record_validation.processed;
  data["success"] = result.success;
  data["failure"] = result.failure;
  data["periods"] = result.periods;
  data["duplicate_periods"] = result.duplicate_periods;
  data["workspace_conflict_periods"] = result.workspace_conflict_periods;
  data["db_conflict_periods"] = result.db_conflict_periods;
  data["config_validation"] = SerializeConfigBundleValidationReport(result.config_validation);
  data["record_validation"] = SerializeRecordPreviewResult(result.record_validation);
  data["issues"] = SerializeValidationIssues(CollectPreflightIssues(result));
  return data;
}

auto ParseStringArrayParam(const Json& params, std::string_view field_name,
                           std::vector<std::string>& values,
                           std::string& error_message) -> bool {
  if (!params.contains(field_name)) {
    values.clear();
    return true;
  }

  const Json& field = params.at(field_name);
  if (!field.is_array()) {
    error_message = "'" + std::string(field_name) + "' must be an array of strings.";
    return false;
  }

  values.clear();
  values.reserve(field.size());
  for (const auto& item : field) {
    if (!item.is_string()) {
      error_message =
          "'" + std::string(field_name) + "' must contain only strings.";
      return false;
    }
    values.push_back(item.get<std::string>());
  }
  return true;
}

auto BuildRecordPreviewResult(const Json& params)
    -> RecordTemplateResult<RecordPreviewResult> {
  const std::string input_path = params.value("input_path", "");
  const std::string config_dir = params.value("config_dir", "");
  if (!config_dir.empty()) {
    return RecordTemplateService::ValidateRecordBatch(input_path, config_dir);
  }

  const std::string validator_config_path =
      params.value("validator_config_path", "");
  const std::string modifier_config_path =
      params.value("modifier_config_path", "");
  if (validator_config_path.empty() || modifier_config_path.empty()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kRequest,
        "Provide either config_dir or both validator_config_path and "
        "modifier_config_path."));
  }

  return RecordTemplateService::ValidateRecordBatch(
      input_path, validator_config_path, modifier_config_path);
}

auto HandleRecordValidationRequest(const Json& request,
                                   std::string_view command_name,
                                   std::string_view success_message,
                                   std::string_view failure_message)
    -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return core_abi::make_response(false,
                                   core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  const std::string input_path = params.value("input_path", "");
  if (input_path.empty()) {
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        std::string(command_name) +
            " requires non-empty 'params.input_path'.");
  }

  const auto preview_result = BuildRecordPreviewResult(params);
  if (!preview_result) {
    return MakeServiceErrorResponse(preview_result.error(), failure_message);
  }

  Json data = SerializeRecordPreviewResult(*preview_result);
  if (preview_result->processed == 0U) {
    return core_abi::make_response(
        false, core_abi::error_code::kBusinessNoInputFiles,
        "No .txt files found under input_path.", std::move(data));
  }

  if (preview_result->failure == 0U) {
    return core_abi::make_response(true, core_abi::error_code::kOk,
                                   std::string(success_message),
                                   std::move(data));
  }

  return core_abi::make_response(false,
                                 core_abi::error_code::kBusinessValidationFailed,
                                 std::string(failure_message), std::move(data));
}

}  // namespace

auto handle_record_preview_command(const Json& request) -> std::string {
  return HandleRecordValidationRequest(
      request, "record_preview", "Record preview completed successfully.",
      "One or more files failed preview.");
}

auto handle_validate_config_bundle_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return core_abi::make_response(false,
                                   core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  const std::string config_dir = params.value("config_dir", "");
  const std::string validator_config_path =
      params.value("validator_config_path", "");
  const std::string modifier_config_path =
      params.value("modifier_config_path", "");
  const std::string export_formats_path =
      params.value("export_formats_path", "");
  const std::string validator_config_text =
      params.value("validator_config_text", "");
  const std::string modifier_config_text =
      params.value("modifier_config_text", "");
  const std::string export_formats_text =
      params.value("export_formats_text", "");

  std::expected<ValidatedConfigBundle, ConfigBundleValidationReport> validation =
      std::unexpected(ConfigBundleValidationReport{});

  if (!config_dir.empty()) {
    validation = ConfigBundleValidationService::ValidateFromConfigDir(config_dir);
  } else if (!validator_config_text.empty() || !modifier_config_text.empty() ||
             !export_formats_text.empty()) {
    if (validator_config_text.empty() || modifier_config_text.empty() ||
        export_formats_text.empty()) {
      return core_abi::make_response(
          false, core_abi::error_code::kParamInvalidRequest,
          "Provide all config texts together when using inline validation.");
    }

    validation = ConfigBundleValidationService::ValidateFromTexts(
        validator_config_text, modifier_config_text, export_formats_text,
        params.value("validator_display_path",
                     std::string(constants::kValidatorConfigName)),
        params.value("modifier_display_path",
                     std::string(constants::kModifierConfigName)),
        params.value("export_formats_display_path",
                     std::string(constants::kExportFormatsConfigName)));
  } else {
    if (validator_config_path.empty() || modifier_config_path.empty() ||
        export_formats_path.empty()) {
      return core_abi::make_response(
          false, core_abi::error_code::kParamInvalidRequest,
          "Provide config_dir, all config file paths, or all config texts.");
    }

    validation = ConfigBundleValidationService::ValidateFromFiles(
        validator_config_path, modifier_config_path, export_formats_path);
  }

  Json data = SerializeConfigBundleValidationReport(
      validation ? validation->report : validation.error());
  if (validation) {
    return core_abi::make_response(true, core_abi::error_code::kOk,
                                   "Config bundle validated successfully.",
                                   std::move(data));
  }

  return core_abi::make_response(false,
                                 core_abi::error_code::kBusinessValidationFailed,
                                 "One or more config files are invalid.",
                                 std::move(data));
}

auto handle_validate_record_batch_command(const Json& request) -> std::string {
  return HandleRecordValidationRequest(
      request, "validate_record_batch",
      "Record batch validated successfully.",
      "One or more record files are invalid.");
}

auto handle_preflight_import_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return core_abi::make_response(false,
                                   core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  ImportPreflightRequest preflight_request;
  preflight_request.input_path = params.value("input_path", "");
  preflight_request.config_dir = params.value("config_dir", "");
  preflight_request.validator_config_path =
      params.value("validator_config_path", "");
  preflight_request.modifier_config_path =
      params.value("modifier_config_path", "");
  preflight_request.export_formats_path =
      params.value("export_formats_path", "");

  if (preflight_request.input_path.empty()) {
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        "preflight_import requires non-empty 'params.input_path'.");
  }

  std::string array_error;
  if (!ParseStringArrayParam(params, "existing_workspace_periods",
                             preflight_request.existing_workspace_periods,
                             array_error) ||
      !ParseStringArrayParam(params, "existing_db_periods",
                             preflight_request.existing_db_periods,
                             array_error)) {
    return core_abi::make_response(false,
                                   core_abi::error_code::kParamInvalidRequest,
                                   std::move(array_error));
  }

  const auto preflight_result = ImportPreflightService::Run(preflight_request);
  if (!preflight_result) {
    return MakeServiceErrorResponse(preflight_result.error(),
                                    "Failed to preflight import batch.");
  }

  Json data = SerializeImportPreflightResult(*preflight_result);
  if (preflight_result->record_validation.processed == 0U) {
    return core_abi::make_response(
        false, core_abi::error_code::kBusinessNoInputFiles,
        "No .txt files found under input_path.", std::move(data));
  }

  if (preflight_result->all_clear) {
    return core_abi::make_response(true, core_abi::error_code::kOk,
                                   "Import preflight completed successfully.",
                                   std::move(data));
  }

  return core_abi::make_response(false,
                                 core_abi::error_code::kBusinessValidationFailed,
                                 "Import preflight found one or more issues.",
                                 std::move(data));
}

}  // namespace bills::core::abi
