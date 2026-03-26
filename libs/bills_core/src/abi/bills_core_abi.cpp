#include "abi/bills_core_abi.h"

#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "common/iso_period.hpp"
#include "common/version.hpp"
#include "config/config_bundle_service.hpp"
#include "ingest/bill_workflow_service.hpp"
#include "nlohmann/json.hpp"
#include "record_template/import_preflight_service.hpp"
#include "record_template/record_template_service.hpp"
#include "reporting/report_render_service.hpp"
#include "ingest/json/bills_json_serializer.hpp"
#include "reporting/standard_report/standard_report_assembler.hpp"

namespace {

using Json = nlohmann::ordered_json;

constexpr const char* kAbiVersion = "2.0.0";
constexpr int kResponseSchemaVersion = 3;
constexpr int kCapabilitiesSchemaVersion = 2;
constexpr int kErrorCodeSchemaVersion = 1;

auto allocate_owned_string(const std::string& value) -> const char* {
  char* buffer = static_cast<char*>(std::malloc(value.size() + 1U));
  if (buffer == nullptr) {
    return nullptr;
  }
  std::memcpy(buffer, value.c_str(), value.size() + 1U);
  return buffer;
}

auto make_response(bool ok, std::string code, std::string message,
                   Json data = Json::object()) -> std::string {
  std::string error_layer = "none";
  if (code.rfind("param.", 0) == 0) {
    error_layer = "param";
  } else if (code.rfind("business.", 0) == 0) {
    error_layer = "business";
  } else if (code.rfind("system.", 0) == 0) {
    error_layer = "system";
  }
  Json response;
  response["ok"] = ok;
  response["code"] = std::move(code);
  response["message"] = std::move(message);
  response["data"] = std::move(data);
  response["error_layer"] = std::move(error_layer);
  response["abi_version"] = kAbiVersion;
  response["response_schema_version"] = kResponseSchemaVersion;
  response["error_code_schema_version"] = kErrorCodeSchemaVersion;
  return response.dump();
}

auto json_for_validation_issues(const std::vector<ValidationIssue>& issues) -> Json {
  Json items = Json::array();
  for (const auto& issue : issues) {
    items.push_back(Json{{"source_kind", issue.source_kind},
                         {"stage", issue.stage},
                         {"code", issue.code},
                         {"message", issue.message},
                         {"path", issue.path},
                         {"line", issue.line},
                         {"column", issue.column},
                         {"field_path", issue.field_path},
                         {"severity", issue.severity}});
  }
  return items;
}

auto json_for_config_report(const ConfigBundleValidationReport& report) -> Json {
  Json files = Json::array();
  for (const auto& file : report.files) {
    files.push_back(Json{{"source_kind", file.source_kind},
                         {"file_name", file.file_name},
                         {"path", file.path},
                         {"ok", file.ok},
                         {"issues", json_for_validation_issues(file.issues)}});
  }
  return Json{{"processed", report.processed},
              {"success", report.success},
              {"failure", report.failure},
              {"all_valid", report.ok},
              {"files", std::move(files)},
              {"enabled_export_formats", report.enabled_export_formats},
              {"available_export_formats", report.available_export_formats}};
}

auto parse_source_documents(const Json& params, SourceDocumentBatch& documents,
                            std::string& error) -> bool {
  const Json source = params.value("documents", Json::array());
  if (!source.is_array()) {
    error = "'documents' must be an array.";
    return false;
  }
  documents.clear();
  for (const auto& item : source) {
    if (!item.is_object()) {
      error = "'documents' items must be objects.";
      return false;
    }
    const std::string display_path = item.value("display_path", "");
    if (display_path.empty()) {
      error = "document.display_path must be non-empty.";
      return false;
    }
    documents.push_back(SourceDocument{
        .display_path = display_path,
        .text = item.value("text", ""),
    });
  }
  return true;
}

auto parse_validator_document(const Json& value, ValidatorConfigDocument& document,
                              std::string& error) -> bool {
  if (!value.is_object()) {
    error = "'validator_document' must be an object.";
    return false;
  }
  document = ValidatorConfigDocument{};
  document.parsed = true;
  document.display_path = value.value("display_path", "validator_config.toml");
  const Json categories = value.value("categories", Json::array());
  if (!categories.is_array()) {
    document.has_categories_array = false;
    return true;
  }
  for (const auto& category_value : categories) {
    ValidatorCategoryDocument category;
    category.is_table = category_value.is_object();
    if (category_value.is_object()) {
      if (category_value.contains("parent_item") && category_value["parent_item"].is_string()) {
        category.parent_item = category_value["parent_item"].get<std::string>();
      }
      if (category_value.contains("description") && category_value["description"].is_string()) {
        category.description = category_value["description"].get<std::string>();
      }
      const Json sub_items = category_value.value("sub_items", Json::array());
      if (!sub_items.is_array()) {
        category.has_sub_items_array = false;
      } else {
        for (const auto& sub_item : sub_items) {
          category.sub_items.push_back(sub_item.is_string()
                                           ? std::optional<std::string>(sub_item.get<std::string>())
                                           : std::nullopt);
        }
      }
    }
    document.categories.push_back(std::move(category));
  }
  return true;
}

auto parse_modifier_document(const Json& value, ModifierConfigDocument& document,
                             std::string& error) -> bool {
  if (!value.is_object()) {
    error = "'modifier_document' must be an object.";
    return false;
  }
  document = ModifierConfigDocument{};
  document.parsed = true;
  document.display_path = value.value("display_path", "modifier_config.toml");
  if (value.contains("auto_renewal_rules")) {
    const Json auto_renewal = value["auto_renewal_rules"];
    document.auto_renewal_is_table = auto_renewal.is_object();
    if (auto_renewal.is_object()) {
      if (auto_renewal.contains("enabled") && auto_renewal["enabled"].is_boolean()) {
        document.auto_renewal_enabled = auto_renewal["enabled"].get<bool>();
      }
      const Json rules = auto_renewal.value("rules", Json::array());
      if (!rules.is_array()) {
        document.auto_renewal_rules_is_array = false;
      } else {
        for (const auto& rule_value : rules) {
          ModifierRuleDocument rule;
          rule.is_table = rule_value.is_object();
          if (rule_value.is_object()) {
            if (rule_value.contains("header_location") &&
                rule_value["header_location"].is_string()) {
              rule.header_location =
                  rule_value["header_location"].get<std::string>();
            }
            if (rule_value.contains("amount") && rule_value["amount"].is_number()) {
              rule.amount = rule_value["amount"].get<double>();
            }
            if (rule_value.contains("description") &&
                rule_value["description"].is_string()) {
              rule.description = rule_value["description"].get<std::string>();
            }
          }
          document.auto_renewal_rules.push_back(std::move(rule));
        }
      }
    }
  }
  const Json metadata_prefixes = value.value("metadata_prefixes", Json::array());
  if (!metadata_prefixes.is_array()) {
    document.metadata_prefixes_is_array = false;
  } else {
    for (const auto& prefix : metadata_prefixes) {
      document.metadata_prefixes.push_back(
          prefix.is_string() ? std::optional<std::string>(prefix.get<std::string>())
                             : std::nullopt);
    }
  }
  const Json display_name_maps = value.value("display_name_maps", Json::object());
  if (!display_name_maps.is_object()) {
    document.display_name_maps_is_table = false;
  } else {
    for (const auto& [map_key, lang_map] : display_name_maps.items()) {
      if (!lang_map.is_object()) {
        document.display_name_maps[map_key] = std::nullopt;
        continue;
      }
      std::map<std::string, std::optional<std::string>> parsed_lang_map;
      for (const auto& [lang_key, lang_value] : lang_map.items()) {
        parsed_lang_map[lang_key] =
            lang_value.is_string() ? std::optional<std::string>(lang_value.get<std::string>())
                                   : std::nullopt;
      }
      document.display_name_maps[map_key] = std::move(parsed_lang_map);
    }
  }
  return true;
}

auto parse_export_document(const Json& value, ExportFormatsDocument& document,
                           std::string& error) -> bool {
  if (!value.is_object()) {
    error = "'export_formats_document' must be an object.";
    return false;
  }
  document = ExportFormatsDocument{};
  document.parsed = true;
  document.display_path = value.value("display_path", "export_formats.toml");
  const Json enabled_formats = value.value("enabled_formats", Json::array());
  if (!enabled_formats.is_array()) {
    document.has_enabled_formats_array = false;
    return true;
  }
  for (const auto& format : enabled_formats) {
    document.enabled_formats.push_back(
        format.is_string() ? std::optional<std::string>(format.get<std::string>())
                           : std::nullopt);
  }
  return true;
}

auto parse_config_documents(const Json& params, ConfigDocumentBundle& documents,
                            std::string& error) -> bool {
  if (!params.contains("validator_document") ||
      !params.contains("modifier_document") ||
      !params.contains("export_formats_document")) {
    error =
        "'validator_document', 'modifier_document', and 'export_formats_document' are required.";
    return false;
  }
  return parse_validator_document(params.at("validator_document"), documents.validator,
                                  error) &&
         parse_modifier_document(params.at("modifier_document"), documents.modifier,
                                 error) &&
         parse_export_document(params.at("export_formats_document"),
                               documents.export_formats, error);
}

class InMemoryBillRepository final : public BillRepository {
 public:
  void InsertBill(const ParsedBill& bill_data) override { bills.push_back(bill_data); }
  std::vector<ParsedBill> bills;
};

auto json_for_batch_result(const BillWorkflowBatchResult& result) -> Json {
  Json files = Json::array();
  for (const auto& file : result.files) {
    Json item{{"path", file.display_path},
              {"ok", file.ok},
              {"stage", file.stage},
              {"stage_group", file.stage_group},
              {"error", file.error},
              {"year", file.year},
              {"month", file.month},
              {"transaction_count", file.transaction_count}};
    item["issues"] = json_for_validation_issues(file.issues);
    if (!file.serialized_json.empty()) {
      item["serialized_json"] = file.serialized_json;
    }
    files.push_back(std::move(item));
  }
  return Json{{"processed", result.processed},
              {"success", result.success},
              {"failure", result.failure},
              {"files", std::move(files)}};
}

auto build_standard_report_payload(const std::vector<ParsedBill>& bills,
                                   std::string_view query_type,
                                   std::string_view query_value) -> Json {
  if (query_type == "year") {
    const auto parsed_year = bills::core::common::iso_period::parse_year(query_value);
    YearlyReportData report;
    report.year = parsed_year.value_or(0);
    std::size_t matched_bills = 0;
    for (const auto& bill : bills) {
      if (bill.year != report.year) {
        continue;
      }
      ++matched_bills;
      report.data_found = true;
      auto& summary = report.monthly_summary[bill.month];
      summary.income += bill.total_income;
      summary.expense += bill.total_expense;
      report.total_income += bill.total_income;
      report.total_expense += bill.total_expense;
    }
    report.balance = report.total_income + report.total_expense;
    const auto standard_report = StandardReportAssembler::FromYearly(report);
    Json data{{"query_type", "year"},
              {"query_value", std::string(query_value)},
              {"year", report.year},
              {"matched_bills", matched_bills},
              {"total_income", report.total_income},
              {"total_expense", report.total_expense},
              {"balance", report.balance},
              {"standard_report", Json::parse(ReportRenderService::Render(standard_report, "json"))},
              {"report_markdown", ReportRenderService::Render(standard_report, "md")}};
    Json monthly = Json::array();
    for (const auto& [month, summary] : report.monthly_summary) {
      monthly.push_back(Json{{"month", month},
                             {"income", summary.income},
                             {"expense", summary.expense},
                             {"balance", summary.income + summary.expense}});
    }
    data["monthly_summary"] = std::move(monthly);
    return data;
  }

  const auto parsed_month =
      bills::core::common::iso_period::parse_year_month(query_value);
  MonthlyReportData report;
  report.year = parsed_month ? parsed_month->year : 0;
  report.month = parsed_month ? parsed_month->month : 0;
  std::size_t matched_bills = 0;
  std::size_t transaction_count = 0;
  for (const auto& bill : bills) {
    if (bill.year != report.year || bill.month != report.month) {
      continue;
    }
    ++matched_bills;
    report.data_found = true;
    report.remark = bill.remark;
    report.total_income += bill.total_income;
    report.total_expense += bill.total_expense;
    report.balance += bill.balance;
    transaction_count += bill.transactions.size();
    for (const auto& transaction : bill.transactions) {
      auto& parent = report.aggregated_data[transaction.parent_category];
      parent.parent_total += transaction.amount;
      auto& sub = parent.sub_categories[transaction.sub_category];
      sub.sub_total += transaction.amount;
      sub.transactions.push_back(transaction);
    }
  }
  const auto standard_report = StandardReportAssembler::FromMonthly(report);
  return Json{{"query_type", "month"},
              {"query_value", std::string(query_value)},
              {"year", report.year},
              {"month", report.month},
              {"matched_bills", matched_bills},
              {"transaction_count", transaction_count},
              {"total_income", report.total_income},
              {"total_expense", report.total_expense},
              {"balance", report.balance},
              {"remark", report.remark},
              {"standard_report", Json::parse(ReportRenderService::Render(standard_report, "json"))},
              {"report_markdown", ReportRenderService::Render(standard_report, "md")}};
}

}  // namespace

auto bills_core_get_abi_version() -> const char* { return kAbiVersion; }

auto bills_core_get_capabilities_json() -> const char* {
  Json data{{"abi_version", kAbiVersion},
            {"capabilities_schema_version", kCapabilitiesSchemaVersion},
            {"response_schema_version", kResponseSchemaVersion},
            {"error_code_schema_version", kErrorCodeSchemaVersion},
            {"supported_commands",
             Json::array({"version", "capabilities", "ping",
                          "validate_config_bundle", "template_generate",
                          "validate_record_batch", "preflight_import",
                          "validate", "convert", "ingest", "import", "query"})},
            {"error_layers", Json::array({"none", "param", "business", "system"})}};
  return allocate_owned_string(data.dump());
}

auto bills_core_invoke_json(const char* request_json_utf8) -> const char* {
  if (request_json_utf8 == nullptr) {
    return allocate_owned_string(
        make_response(false, "param.invalid_argument",
                      "request_json_utf8 must not be null."));
  }

  Json request;
  try {
    request = Json::parse(request_json_utf8);
  } catch (const std::exception& ex) {
    Json data;
    data["parse_error"] = ex.what();
    return allocate_owned_string(
        make_response(false, "param.invalid_json",
                      "Failed to parse JSON request.", std::move(data)));
  }

  if (!request.is_object()) {
    return allocate_owned_string(
        make_response(false, "param.invalid_request",
                      "Request root must be a JSON object."));
  }

  const std::string command = request.value("command", "");
  const Json params = request.value("params", Json::object());
  if (command.empty()) {
    return allocate_owned_string(
        make_response(false, "param.invalid_request",
                      "Request must contain non-empty string field 'command'."));
  }
  if (!params.is_object()) {
    return allocate_owned_string(
        make_response(false, "param.invalid_request",
                      "'params' must be a JSON object."));
  }

  if (command == "version") {
    return allocate_owned_string(make_response(
        true, "ok", "ABI version returned.",
        Json{{"abi_version", kAbiVersion},
             {"response_schema_version", kResponseSchemaVersion},
             {"capabilities_schema_version", kCapabilitiesSchemaVersion},
             {"error_code_schema_version", kErrorCodeSchemaVersion}}));
  }
  if (command == "capabilities") {
    const char* payload = bills_core_get_capabilities_json();
    std::string response(payload == nullptr ? "{}" : payload);
    bills_core_free_string(payload);
    return allocate_owned_string(response);
  }
  if (command == "ping") {
    return allocate_owned_string(make_response(
        true, "ok", "Ping handled.",
        Json{{"pong", true},
             {"abi_version", kAbiVersion},
             {"response_schema_version", kResponseSchemaVersion},
             {"echo", request.value("payload", Json::object())}}));
  }

  std::string error;
  ConfigDocumentBundle documents;
  const bool needs_config =
      command == "validate_config_bundle" || command == "template_generate" ||
      command == "validate_record_batch" || command == "preflight_import" ||
      command == "validate" || command == "convert" || command == "ingest";
  std::optional<ValidatedConfigBundle> validated;
  if (needs_config && command != "template_generate") {
    if (!parse_config_documents(params, documents, error)) {
      return allocate_owned_string(
          make_response(false, "param.invalid_request", error));
    }
    const auto validation = ConfigBundleService::Validate(documents);
    if (!validation) {
      return allocate_owned_string(make_response(
          false, "business.validation_failed", "Config bundle validation failed.",
          json_for_config_report(validation.error())));
    }
    validated = *validation;
  }

  if (command == "validate_config_bundle") {
    return allocate_owned_string(
        make_response(true, "ok", "Config bundle validated successfully.",
                      json_for_config_report(validated->report)));
  }

  if (command == "template_generate") {
    ValidatorConfigDocument validator_document;
    if (!params.contains("validator_document") ||
        !parse_validator_document(params.at("validator_document"), validator_document,
                                 error)) {
      return allocate_owned_string(make_response(
          false, "param.invalid_request",
          error.empty() ? "'validator_document' is required." : error));
    }
    const auto layout =
        RecordTemplateService::BuildOrderedTemplateLayout(validator_document);
    if (!layout) {
      return allocate_owned_string(make_response(
          false, "param.invalid_request", layout.error().message));
    }
    TemplateGenerationRequest request_data;
    request_data.period = params.value("period", "");
    request_data.start_period = params.value("start_period", "");
    request_data.end_period = params.value("end_period", "");
    request_data.start_year = params.value("start_year", "");
    request_data.end_year = params.value("end_year", "");
    request_data.layout = *layout;
    const auto result = RecordTemplateService::GenerateTemplates(request_data);
    if (!result) {
      return allocate_owned_string(make_response(
          false, "param.invalid_request", result.error().message));
    }
    Json templates = Json::array();
    for (const auto& item : result->templates) {
      templates.push_back(Json{{"period", item.period},
                               {"relative_path", item.relative_path},
                               {"text", item.text}});
    }
    return allocate_owned_string(make_response(
        true, "ok", "Templates generated successfully.",
        Json{{"generated", result->templates.size()},
             {"templates", std::move(templates)}}));
  }

  if (command == "validate" || command == "convert" || command == "ingest" ||
      command == "validate_record_batch") {
    SourceDocumentBatch source_documents;
    if (!parse_source_documents(params, source_documents, error)) {
      return allocate_owned_string(
          make_response(false, "param.invalid_request", error));
    }
    if (command == "validate_record_batch") {
      const auto preview = RecordTemplateService::ValidateRecordBatch(
          source_documents, validated->runtime_config);
      if (!preview) {
        return allocate_owned_string(make_response(
            false, "system.native_failure", preview.error().message));
      }
      const bool ok = preview->failure == 0U;
      return allocate_owned_string(make_response(
          ok, ok ? "ok" : "business.validation_failed",
          ok ? "Record batch validated successfully."
             : "One or more documents failed validation.",
          Json{{"processed", preview->processed},
               {"success", preview->success},
               {"failure", preview->failure},
               {"all_valid", ok},
               {"periods", preview->periods},
               {"files", [&preview]() {
                  Json files = Json::array();
                  for (const auto& file : preview->files) {
                    files.push_back(Json{
                        {"path", file.path},
                        {"ok", file.ok},
                        {"period", file.period},
                        {"year", file.year},
                        {"month", file.month},
                        {"transaction_count", file.transaction_count},
                        {"total_income", file.total_income},
                        {"total_expense", file.total_expense},
                        {"balance", file.balance},
                        {"error", file.error},
                        {"issues", json_for_validation_issues(file.issues)},
                    });
                  }
                  return files;
                }()}}));
    }
    const bool include_serialized_json =
        params.value("include_serialized_json", false);
    if (command == "validate") {
      const auto result =
          BillWorkflowService::Validate(source_documents, validated->runtime_config);
      const bool ok = result.failure == 0U;
      return allocate_owned_string(make_response(
          ok, ok ? "ok" : "business.validation_failed",
          ok ? "Documents validated successfully."
             : "One or more documents failed validation.",
          json_for_batch_result(result)));
    }
    if (command == "convert") {
      const auto result = BillWorkflowService::Convert(
          source_documents, validated->runtime_config, include_serialized_json);
      const bool ok = result.failure == 0U;
      return allocate_owned_string(make_response(
          ok, ok ? "ok" : "business.convert_failed",
          ok ? "Documents converted successfully."
             : "One or more documents failed conversion.",
          json_for_batch_result(result)));
    }
    InMemoryBillRepository repository;
    const auto result = BillWorkflowService::Ingest(
        source_documents, validated->runtime_config, repository,
        include_serialized_json);
    const bool ok = result.failure == 0U;
    Json data = json_for_batch_result(result);
    data["imported"] = result.success;
    data["all_ingested"] = ok;
    data["repository_mode"] = "memory";
    return allocate_owned_string(make_response(
        ok, ok ? "ok" : "business.ingest_failed",
        ok ? "Documents ingested successfully."
           : "One or more documents failed ingest.",
        std::move(data)));
  }

  if (command == "import") {
    SourceDocumentBatch source_documents;
    if (!parse_source_documents(params, source_documents, error)) {
      return allocate_owned_string(
          make_response(false, "param.invalid_request", error));
    }
    InMemoryBillRepository repository;
    const auto result = BillWorkflowService::ImportJson(source_documents, repository);
    const bool ok = result.failure == 0U;
    Json data = json_for_batch_result(result);
    data["imported"] = result.success;
    data["all_imported"] = ok;
    data["repository_mode"] = "memory";
    return allocate_owned_string(make_response(
        ok, ok ? "ok" : "business.import_failed",
        ok ? "JSON documents imported successfully."
           : "One or more JSON documents failed import.",
        std::move(data)));
  }

  if (command == "query") {
    SourceDocumentBatch source_documents;
    if (!parse_source_documents(params, source_documents, error)) {
      return allocate_owned_string(
          make_response(false, "param.invalid_request", error));
    }
    std::vector<ParsedBill> bills;
    std::size_t parse_failures = 0;
    for (const auto& document : source_documents) {
      try {
        bills.push_back(BillJsonSerializer::deserialize(document.text));
      } catch (...) {
        ++parse_failures;
      }
    }
    const std::string query_type = params.value("type", "");
    const std::string query_value = params.value("value", "");
    if (query_type != "year" && query_type != "y" &&
        query_type != "month" && query_type != "m") {
      return allocate_owned_string(
          make_response(false, "param.invalid_request", "Unsupported query type."));
    }
    const bool is_year = query_type == "year" || query_type == "y";
    const auto year_ok = bills::core::common::iso_period::parse_year(query_value).has_value();
    const auto month_ok =
        bills::core::common::iso_period::parse_year_month(query_value).has_value();
    if ((is_year && !year_ok) || (!is_year && !month_ok)) {
      return allocate_owned_string(make_response(
          false, "param.invalid_request",
          is_year ? "Year query requires YYYY." : "Month query requires YYYY-MM.",
          Json{{"expected_format", is_year ? "YYYY" : "YYYY-MM"}}));
    }
    Json data = build_standard_report_payload(bills, is_year ? "year" : "month",
                                              query_value);
    data["processed"] = source_documents.size();
    data["parse_failures"] = parse_failures;
    const bool ok = static_cast<std::size_t>(data["matched_bills"]) > 0U;
    return allocate_owned_string(make_response(
        ok, ok ? "ok" : "business.no_input_files",
        ok ? (is_year ? "Year query completed successfully."
                      : "Month query completed successfully.")
           : "No matching bills found.",
        std::move(data)));
  }

  if (command == "preflight_import") {
    SourceDocumentBatch source_documents;
    if (!parse_source_documents(params, source_documents, error)) {
      return allocate_owned_string(
          make_response(false, "param.invalid_request", error));
    }
    ImportPreflightRequest preflight_request;
    preflight_request.documents = source_documents;
    preflight_request.config_validation = validated->report;
    preflight_request.config_bundle = validated->runtime_config;
    if (params.contains("existing_workspace_periods") &&
        params["existing_workspace_periods"].is_array()) {
      preflight_request.existing_workspace_periods =
          params["existing_workspace_periods"].get<std::vector<std::string>>();
    }
    if (params.contains("existing_db_periods") &&
        params["existing_db_periods"].is_array()) {
      preflight_request.existing_db_periods =
          params["existing_db_periods"].get<std::vector<std::string>>();
    }
    const auto result = ImportPreflightService::Run(preflight_request);
    if (!result) {
      return allocate_owned_string(make_response(
          false, "system.native_failure", result.error().message));
    }
    const bool ok = result->all_clear;
    return allocate_owned_string(make_response(
        ok, ok ? "ok" : "business.validation_failed",
        ok ? "Import preflight completed successfully."
           : "Import preflight found blocking issues.",
        Json{{"all_clear", result->all_clear},
             {"duplicate_periods", result->duplicate_periods},
             {"workspace_conflict_periods", result->workspace_conflict_periods},
             {"db_conflict_periods", result->db_conflict_periods},
             {"periods", result->periods}}));
  }

  return allocate_owned_string(
      make_response(false, "param.unknown_command", "Command is not supported."));
}

void bills_core_free_string(const char* owned_utf8_str) {
  std::free(const_cast<char*>(owned_utf8_str));
}
