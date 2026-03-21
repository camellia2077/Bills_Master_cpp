#include <jni.h>
#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "abi/bills_core_abi.h"
#include "application/use_cases/workflow_use_case.hpp"
#include "bills_io/io_factory.hpp"
#include "common/Result.hpp"
#include "common/version.hpp"
#include "nlohmann/json.hpp"
#include "ports/output_path_builder.hpp"
#include "record_template/record_template_service.hpp"
#include "reports/core/standard_report_renderer_registry.hpp"
#include "reports/standard_json/standard_report_assembler.hpp"

namespace fs = std::filesystem;

namespace {

using Json = nlohmann::ordered_json;

struct QueryMonth {
  int year = 0;
  int month = 0;
};

auto is_ascii_digits(std::string_view value) -> bool {
  return !value.empty() &&
         std::all_of(value.begin(), value.end(), [](unsigned char ch) -> bool {
           return std::isdigit(ch) != 0;
         });
}

auto make_response(bool ok, std::string code, std::string message,
                   Json data = Json::object()) -> std::string {
  Json response;
  response["ok"] = ok;
  response["code"] = std::move(code);
  response["message"] = std::move(message);
  response["data"] = std::move(data);
  return response.dump(2);
}

auto invoke_core_abi(const Json& request) -> std::string {
  const std::string request_json = request.dump();
  const char* raw_response = bills_core_invoke_json(request_json.c_str());
  if (raw_response == nullptr) {
    return make_response(false, "system.native_failure",
                         "Failed to allocate ABI response.");
  }

  std::string response(raw_response);
  bills_core_free_string(raw_response);
  return response;
}

auto from_jstring(JNIEnv* env, jstring value) -> std::string {
  if (value == nullptr) {
    return {};
  }
  const char* raw = env->GetStringUTFChars(value, nullptr);
  if (raw == nullptr) {
    return {};
  }
  std::string text(raw);
  env->ReleaseStringUTFChars(value, raw);
  return text;
}

auto to_jstring(JNIEnv* env, const std::string& value) -> jstring {
  return env->NewStringUTF(value.c_str());
}

auto count_matching_year_bills(sqlite3* db_connection, std::string_view iso_year)
    -> int {
  const char* sql = "SELECT COUNT(*) FROM bills WHERE substr(bill_date, 1, 4) = ?;";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_connection, sql, -1, &statement, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare bill count query.");
  }

  sqlite3_bind_text(statement, 1, iso_year.data(),
                    static_cast<int>(iso_year.size()), SQLITE_TRANSIENT);

  int result = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    result = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  return result;
}

auto count_matching_month_bills(sqlite3* db_connection,
                                std::string_view iso_month) -> int {
  const char* sql = "SELECT COUNT(*) FROM bills WHERE bill_date = ?;";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_connection, sql, -1, &statement, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare bill count query.");
  }

  sqlite3_bind_text(statement, 1, iso_month.data(),
                    static_cast<int>(iso_month.size()), SQLITE_TRANSIENT);

  int result = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    result = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  return result;
}

auto count_transactions(const MonthlyReportData& report) -> std::size_t {
  std::size_t count = 0;
  for (const auto& [_, parent] : report.aggregated_data) {
    for (const auto& [__, sub] : parent.sub_categories) {
      count += sub.transactions.size();
    }
  }
  return count;
}

auto parse_iso_year(const std::string& value) -> std::optional<int> {
  if (value.size() != 4U || !is_ascii_digits(value)) {
    return std::nullopt;
  }

  const int year = std::stoi(value);
  if (year < 1900 || year > 9999) {
    return std::nullopt;
  }
  return year;
}

auto parse_iso_month(const std::string& value) -> std::optional<QueryMonth> {
  if (value.size() != 7U || value[4] != '-' ||
      !is_ascii_digits(std::string_view(value).substr(0, 4)) ||
      !is_ascii_digits(std::string_view(value).substr(5, 2))) {
    return std::nullopt;
  }
  QueryMonth parsed;
  parsed.year = std::stoi(value.substr(0, 4));
  parsed.month = std::stoi(value.substr(5, 2));
  if (parsed.year < 1900 || parsed.year > 9999 ||
      parsed.month < 1 || parsed.month > 12) {
    return std::nullopt;
  }
  return parsed;
}

auto json_for_monthly_summary(const std::map<int, MonthlySummary>& monthly_summary)
    -> Json {
  Json summary = Json::array();
  for (const auto& [month, value] : monthly_summary) {
    Json entry;
    entry["month"] = month;
    entry["income"] = value.income;
    entry["expense"] = value.expense;
    entry["balance"] = value.income + value.expense;
    summary.push_back(std::move(entry));
  }
  return summary;
}

auto json_for_failure_detail(const ProcessFailureDetail& detail) -> Json {
  Json item;
  item["path"] = detail.path;
  item["stage"] = detail.stage;
  item["message"] = detail.message;
  return item;
}

auto json_for_failure_details(
    const std::vector<ProcessFailureDetail>& details) -> Json {
  Json items = Json::array();
  for (const auto& detail : details) {
    items.push_back(json_for_failure_detail(detail));
  }
  return items;
}

auto summarize_failure_detail(const ProcessFailureDetail& detail) -> std::string {
  if (detail.stage.empty()) {
    return detail.message;
  }
  if (detail.message.empty()) {
    return detail.stage;
  }
  return detail.stage + ": " + detail.message;
}

auto json_for_record_preview_file(const RecordPreviewFile& file) -> Json {
  Json item;
  item["path"] = file.path.string();
  item["ok"] = file.ok;
  if (file.ok) {
    item["period"] = file.period;
    item["year"] = file.year;
    item["month"] = file.month;
    item["transaction_count"] = file.transaction_count;
    item["total_income"] = file.total_income;
    item["total_expense"] = file.total_expense;
    item["balance"] = file.balance;
  } else {
    item["error"] = file.error;
  }
  return item;
}

auto json_for_invalid_record_file(const InvalidPeriodFile& invalid_file) -> Json {
  Json item;
  item["path"] = invalid_file.path.string();
  item["error"] = invalid_file.error;
  return item;
}

auto make_record_template_failure_response(const RecordTemplateError& error,
                                           std::string_view fallback_message)
    -> std::string {
  std::string code = "system.native_failure";
  if (error.category == RecordTemplateErrorCategory::kRequest ||
      error.category == RecordTemplateErrorCategory::kConfig ||
      error.category == RecordTemplateErrorCategory::kInputPath ||
      error.category == RecordTemplateErrorCategory::kOutputPath) {
    code = "param.invalid_argument";
  }

  Json data;
  data["detail"] = FormatRecordTemplateError(error);
  return make_response(false, std::move(code),
                       error.message.empty() ? std::string(fallback_message)
                                             : error.message,
                       std::move(data));
}

auto attach_rendered_report_payload(Json& data,
                                    const StandardReport& standard_report)
    -> void {
  if (StandardReportRendererRegistry::IsFormatAvailable("json")) {
    data["standard_report"] = Json::parse(
        StandardReportRendererRegistry::Render(standard_report, "json"));
  }
  if (StandardReportRendererRegistry::IsFormatAvailable("md")) {
    data["report_markdown"] =
        StandardReportRendererRegistry::Render(standard_report, "md");
  }
}

auto load_config_bundle(const std::string& config_dir) -> ConfigBundle {
  auto provider = bills::io::CreateConfigProvider();
  const auto config = provider->Load(config_dir);
  if (!config) {
    throw std::runtime_error(FormatError(config.error()));
  }
  return *config;
}

auto load_table_columns(sqlite3* db_connection, const std::string& table_name)
    -> std::set<std::string> {
  sqlite3_stmt* statement = nullptr;
  const std::string sql = "PRAGMA table_info(" + table_name + ");";
  if (sqlite3_prepare_v2(db_connection, sql.c_str(), -1, &statement, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to inspect database schema.");
  }

  std::set<std::string> columns;
  while (sqlite3_step(statement) == SQLITE_ROW) {
    const auto* name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    if (name != nullptr) {
      columns.emplace(name);
    }
  }
  sqlite3_finalize(statement);
  return columns;
}

auto contains_required_columns(const std::set<std::string>& columns,
                               const std::vector<std::string>& required_columns)
    -> bool {
  for (const auto& column : required_columns) {
    if (!columns.contains(column)) {
      return false;
    }
  }
  return true;
}

auto remove_database_family(const fs::path& db_path) -> void {
  std::error_code error;
  fs::remove(db_path, error);
  fs::remove(db_path.string() + "-wal", error);
  fs::remove(db_path.string() + "-shm", error);
}

auto reset_legacy_database_if_needed(const std::string& db_path) -> bool {
  const fs::path database_path(db_path);
  if (!fs::exists(database_path)) {
    return false;
  }

  sqlite3* db_connection = nullptr;
  const int open_result = sqlite3_open_v2(
      db_path.c_str(), &db_connection, SQLITE_OPEN_READONLY, nullptr);
  if (open_result != SQLITE_OK) {
    if (db_connection != nullptr) {
      sqlite3_close(db_connection);
    }
    remove_database_family(database_path);
    return true;
  }

  bool should_reset = false;
  try {
    const auto bills_columns = load_table_columns(db_connection, "bills");
    const auto transactions_columns =
        load_table_columns(db_connection, "transactions");

    const std::vector<std::string> required_bills_columns = {
        "bill_date", "year", "month", "remark", "total_income",
        "total_expense", "balance"};
    const std::vector<std::string> required_transaction_columns = {
        "bill_id", "parent_category", "sub_category", "description", "amount",
        "source", "comment", "transaction_type"};

    should_reset =
        !contains_required_columns(bills_columns, required_bills_columns) ||
        !contains_required_columns(transactions_columns,
                                   required_transaction_columns);
  } catch (...) {
    should_reset = true;
  }

  sqlite3_close(db_connection);
  if (should_reset) {
    remove_database_family(database_path);
  }
  return should_reset;
}

auto import_sample(const std::string& input_path, const std::string& config_dir,
                   const std::string& db_path) -> std::string {
  if (input_path.empty() || config_dir.empty() || db_path.empty()) {
    return make_response(false, "param.invalid_argument",
                         "inputPath, configDir, and dbPath must be non-empty.");
  }
  if (!fs::exists(input_path)) {
    Json data;
    data["input_path"] = input_path;
    return make_response(false, "param.invalid_argument",
                         "Bundled sample input path does not exist.", std::move(data));
  }

  const ConfigBundle config_bundle = load_config_bundle(config_dir);
  auto content_reader = bills::io::CreateBillContentReader();
  auto file_enumerator = bills::io::CreateBillFileEnumerator();
  auto serializer = bills::io::CreateBillSerializer();
  const fs::path db_parent = fs::path(db_path).parent_path();
  std::error_code create_error;
  fs::create_directories(db_parent, create_error);
  if (create_error) {
    Json data;
    data["db_path"] = db_path;
    return make_response(false, "system.io_failure",
                         "Failed to prepare the database directory.",
                         std::move(data));
  }
  const bool database_reset = reset_legacy_database_if_needed(db_path);
  auto output_path_builder = bills::io::CreateYearPartitionOutputPathBuilder(
      (db_parent / "json_cache").string());
  auto repository = bills::io::CreateBillRepository(db_path);

  WorkflowUseCase use_case(config_bundle.validator_config,
                           config_bundle.modifier_config, *content_reader,
                           *file_enumerator, *serializer, *output_path_builder);
  const auto result = use_case.Ingest(input_path, *repository, false);
  if (!result) {
    Json data;
    data["input_path"] = input_path;
    data["db_path"] = db_path;
    return make_response(false, "business.import_failed",
                         FormatError(result.error()), std::move(data));
  }

  Json data;
  data["input_path"] = input_path;
  data["config_dir"] = config_dir;
  data["db_path"] = db_path;
  data["database_reset"] = database_reset;
  data["processed"] = result->success + result->failure;
  data["success"] = result->success;
  data["failure"] = result->failure;
  data["imported"] = result->success;
  if (!result->failure_details.empty()) {
    data["first_failure"] =
        json_for_failure_detail(result->failure_details.front());
    data["failures"] = json_for_failure_details(result->failure_details);
  }
  if (result->failure == 0) {
    return make_response(true, "ok", "Sample import finished.",
                         std::move(data));
  }

  std::string message =
      result->success == 0
          ? "Sample import failed before any bill was written."
          : "Sample import finished with partial failures.";
  if (!result->failure_details.empty()) {
    const std::string detail_summary =
        summarize_failure_detail(result->failure_details.front());
    if (!detail_summary.empty()) {
      message = result->success == 0 ? detail_summary
                                     : "Partial import failure: " + detail_summary;
    }
  }
  return make_response(false, "business.import_failed", message,
                       std::move(data));
}

auto query_year(const std::string& db_path, const std::string& iso_year)
    -> std::string {
  if (db_path.empty()) {
    return make_response(false, "param.invalid_argument",
                         "dbPath must be non-empty.");
  }
  if (!parse_iso_year(iso_year).has_value()) {
    return make_response(false, "param.invalid_argument",
                         "isoYear must use YYYY.");
  }

  auto db_session = bills::io::CreateReportDbSession(db_path);
  auto report_data_gateway =
      bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
  const YearlyReportData report = report_data_gateway->ReadYearlyData(iso_year);
  if (!report.data_found) {
    Json data;
    data["db_path"] = db_path;
    data["iso_year"] = iso_year;
    return make_response(false, "business.query_not_found",
                         "No data matched the requested year.", std::move(data));
  }

  const auto standard_report = StandardReportAssembler::FromYearly(report);
  Json data;
  data["query_type"] = "year";
  data["query_value"] = iso_year;
  data["year"] = report.year;
  data["matched_bills"] = count_matching_year_bills(
      db_session->GetConnectionHandle(), iso_year);
  data["total_income"] = report.total_income;
  data["total_expense"] = report.total_expense;
  data["balance"] = report.balance;
  data["monthly_summary"] = json_for_monthly_summary(report.monthly_summary);
  attach_rendered_report_payload(data, standard_report);
  return make_response(true, "ok", "Year query completed successfully.",
                       std::move(data));
}

auto query_month(const std::string& db_path, const std::string& iso_month) -> std::string {
  if (db_path.empty()) {
    return make_response(false, "param.invalid_argument",
                         "dbPath must be non-empty.");
  }
  if (!parse_iso_month(iso_month).has_value()) {
    return make_response(false, "param.invalid_argument",
                         "isoMonth must use YYYY-MM.");
  }

  auto db_session = bills::io::CreateReportDbSession(db_path);
  auto report_data_gateway =
      bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
  const MonthlyReportData report =
      report_data_gateway->ReadMonthlyData(iso_month);
  if (!report.data_found) {
    Json data;
    data["db_path"] = db_path;
    data["iso_month"] = iso_month;
    return make_response(false, "business.query_not_found",
                         "No data matched the requested month.", std::move(data));
  }

  const auto standard_report = StandardReportAssembler::FromMonthly(report);
  Json data;
  data["query_type"] = "month";
  data["query_value"] = iso_month;
  data["year"] = report.year;
  data["month"] = report.month;
  data["matched_bills"] = count_matching_month_bills(
      db_session->GetConnectionHandle(), iso_month);
  data["transaction_count"] = count_transactions(report);
  data["total_income"] = report.total_income;
  data["total_expense"] = report.total_expense;
  data["balance"] = report.balance;
  data["remark"] = report.remark;
  attach_rendered_report_payload(data, standard_report);
  return make_response(true, "ok", "Month query completed successfully.",
                       std::move(data));
}

auto generate_record_template(const std::string& config_dir,
                              const std::string& iso_month) -> std::string {
  if (config_dir.empty() || iso_month.empty()) {
    return make_response(false, "param.invalid_argument",
                         "configDir and isoMonth must be non-empty.");
  }

  TemplateGenerationRequest request;
  request.config_dir = config_dir;
  request.period = iso_month;

  const auto result = RecordTemplateService::GenerateTemplates(request);
  if (!result) {
    return make_record_template_failure_response(
        result.error(), "Failed to generate record template.");
  }
  if (result->templates.size() != 1U) {
    return make_response(false, "system.native_failure",
                         "Expected a single generated record template.");
  }

  const auto& generated = result->templates.front();
  Json data;
  data["period"] = generated.period;
  data["relative_path"] = generated.relative_path.generic_string();
  data["text"] = generated.text;
  data["persisted"] = false;
  return make_response(true, "ok", "Record template generated successfully.",
                       std::move(data));
}

auto preview_record_path(const std::string& input_path,
                         const std::string& config_dir) -> std::string {
  if (input_path.empty() || config_dir.empty()) {
    return make_response(false, "param.invalid_argument",
                         "inputPath and configDir must be non-empty.");
  }

  Json request;
  request["command"] = "validate_record_batch";
  request["params"] = {
      {"input_path", input_path},
      {"config_dir", config_dir},
  };
  return invoke_core_abi(request);
}

auto validate_config_bundle(const std::string& validator_config_text,
                            const std::string& modifier_config_text,
                            const std::string& export_formats_text,
                            const std::string& validator_display_path,
                            const std::string& modifier_display_path,
                            const std::string& export_formats_display_path)
    -> std::string {
  if (validator_config_text.empty() || modifier_config_text.empty() ||
      export_formats_text.empty()) {
    return make_response(false, "param.invalid_argument",
                         "All config texts must be non-empty.");
  }

  Json request;
  request["command"] = "validate_config_bundle";
  request["params"] = {
      {"validator_config_text", validator_config_text},
      {"modifier_config_text", modifier_config_text},
      {"export_formats_text", export_formats_text},
      {"validator_display_path", validator_display_path},
      {"modifier_display_path", modifier_display_path},
      {"export_formats_display_path", export_formats_display_path},
  };
  return invoke_core_abi(request);
}

auto list_record_periods(const std::string& input_path) -> std::string {
  if (input_path.empty()) {
    return make_response(false, "param.invalid_argument",
                         "inputPath must be non-empty.");
  }

  const auto result = RecordTemplateService::ListPeriods(input_path);
  if (!result) {
    return make_record_template_failure_response(result.error(),
                                                 "Failed to list record periods.");
  }

  Json data;
  data["input_path"] = result->input_path.string();
  data["processed"] = result->processed;
  data["valid"] = result->valid;
  data["invalid"] = result->invalid;
  data["periods"] = result->periods;
  Json invalid_files = Json::array();
  for (const auto& invalid_file : result->invalid_files) {
    invalid_files.push_back(json_for_invalid_record_file(invalid_file));
  }
  data["invalid_files"] = std::move(invalid_files);

  if (result->processed == 0U) {
    return make_response(true, "ok", "No record files found yet.",
                         std::move(data));
  }
  if (result->invalid == 0U) {
    return make_response(true, "ok", "Record periods listed successfully.",
                         std::move(data));
  }
  return make_response(false, "business.record_periods_incomplete",
                       "One or more record files did not contain a valid period.",
                       std::move(data));
}

auto core_version_info() -> std::string {
  Json data;
  data["version_name"] = std::string(bills::core::version::kVersion);
  data["last_updated"] = std::string(bills::core::version::kLastUpdated);
  return make_response(true, "ok", "Core version loaded successfully.",
                       std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_importBundledSampleNative(
    JNIEnv* env, jclass, jstring input_path, jstring config_dir,
    jstring db_path) {
  try {
    return to_jstring(env, import_sample(from_jstring(env, input_path),
                                         from_jstring(env, config_dir),
                                         from_jstring(env, db_path)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_queryYearNative(
    JNIEnv* env, jclass, jstring db_path, jstring iso_year) {
  try {
    return to_jstring(
        env,
        query_year(from_jstring(env, db_path), from_jstring(env, iso_year)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_queryMonthNative(
    JNIEnv* env, jclass, jstring db_path, jstring iso_month) {
  try {
    return to_jstring(env, query_month(from_jstring(env, db_path),
                                       from_jstring(env, iso_month)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_generateRecordTemplateNative(
    JNIEnv* env, jclass, jstring config_dir, jstring iso_month) {
  try {
    return to_jstring(env, generate_record_template(from_jstring(env, config_dir),
                                                    from_jstring(env, iso_month)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_previewRecordPathNative(
    JNIEnv* env, jclass, jstring input_path, jstring config_dir) {
  try {
    return to_jstring(env, preview_record_path(from_jstring(env, input_path),
                                               from_jstring(env, config_dir)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_validateConfigBundleNative(
    JNIEnv* env, jclass, jstring validator_config_text,
    jstring modifier_config_text, jstring export_formats_text,
    jstring validator_display_path, jstring modifier_display_path,
    jstring export_formats_display_path) {
  try {
    return to_jstring(
        env,
        validate_config_bundle(
            from_jstring(env, validator_config_text),
            from_jstring(env, modifier_config_text),
            from_jstring(env, export_formats_text),
            from_jstring(env, validator_display_path),
            from_jstring(env, modifier_display_path),
            from_jstring(env, export_formats_display_path)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_listRecordPeriodsNative(
    JNIEnv* env, jclass, jstring input_path) {
  try {
    return to_jstring(
        env, list_record_periods(from_jstring(env, input_path)));
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_BillsNativeBindings_coreVersionNative(
    JNIEnv* env, jclass) {
  try {
    return to_jstring(env, core_version_info());
  } catch (const std::exception& error) {
    return to_jstring(
        env, make_response(false, "system.native_failure", error.what()));
  }
}
