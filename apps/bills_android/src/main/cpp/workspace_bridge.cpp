#include <jni.h>
#include <sqlite3.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "bills_io/host_flow_support.hpp"
#include "jni_common.hpp"

namespace fs = std::filesystem;

namespace {

using bills::android::jni::Json;

auto json_for_validation_issue(const ValidationIssue& issue) -> Json {
  Json item;
  item["source_kind"] = issue.source_kind;
  item["stage"] = issue.stage;
  item["code"] = issue.code;
  item["message"] = issue.message;
  item["path"] = issue.path;
  item["line"] = issue.line;
  item["column"] = issue.column;
  item["field_path"] = issue.field_path;
  item["severity"] = issue.severity;
  return item;
}

auto json_for_validation_issues(const std::vector<ValidationIssue>& issues) -> Json {
  Json items = Json::array();
  for (const auto& issue : issues) {
    items.push_back(json_for_validation_issue(issue));
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
              {"transaction_count", file.transaction_count},
              {"issues", json_for_validation_issues(file.issues)}};
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
    const auto* name =
        reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
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
  const int open_result =
      sqlite3_open_v2(db_path.c_str(), &db_connection, SQLITE_OPEN_READONLY, nullptr);
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

auto import_records_to_database(const std::string& config_dir,
                                const std::string& records_dir,
                                const std::string& db_path) -> std::string {
  if (config_dir.empty() || records_dir.empty() || db_path.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "configDir, recordsDir, and dbPath must be non-empty.");
  }
  if (!fs::exists(fs::path(records_dir))) {
    Json data;
    data["records_dir"] = records_dir;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "Records directory does not exist.", std::move(data));
  }

  const fs::path db_parent = fs::path(db_path).parent_path();
  std::error_code create_error;
  fs::create_directories(db_parent, create_error);
  if (create_error) {
    Json data;
    data["db_path"] = db_path;
    return bills::android::jni::MakeResponse(
        false, "system.io_failure",
        "Failed to prepare the database directory.", std::move(data));
  }

  const bool database_reset = reset_legacy_database_if_needed(db_path);
  const auto result =
      bills::io::IngestDocuments(records_dir, config_dir, db_path, false);
  if (!result) {
    Json data;
    data["config_dir"] = config_dir;
    data["records_dir"] = records_dir;
    data["db_path"] = db_path;
    data["database_reset"] = database_reset;
    return bills::android::jni::MakeResponse(
        false, "business.import_failed", FormatError(result.error()),
        std::move(data));
  }

  Json data;
  data["config_dir"] = config_dir;
  data["records_dir"] = records_dir;
  data["db_path"] = db_path;
  data["database_reset"] = database_reset;
  data["processed"] = result->processed;
  data["success"] = result->success;
  data["failure"] = result->failure;
  data["imported"] = result->success;

  Json failures = Json::array();
  for (const auto& file : result->files) {
    if (!file.ok) {
      Json item;
      item["path"] = file.display_path;
      item["stage"] = file.stage;
      item["stage_group"] = file.stage_group;
      item["message"] = file.error;
      item["issues"] = json_for_validation_issues(file.issues);
      failures.push_back(std::move(item));
    }
  }
  if (!failures.empty()) {
    data["first_failure"] = failures.front();
    data["failures"] = std::move(failures);
  }
  if (result->failure == 0U) {
    return bills::android::jni::MakeResponse(
        true, "ok", "Record import finished.", std::move(data));
  }

  std::string message =
      result->success == 0U
          ? "Record import failed before any bill was written."
          : "Record import finished with partial failures.";
  if (!result->files.empty()) {
    const auto first_failure = std::find_if(
        result->files.begin(), result->files.end(),
        [](const BillWorkflowFileResult& file) { return !file.ok; });
    if (first_failure != result->files.end()) {
      const std::string detail_summary =
          first_failure->stage.empty() ? first_failure->error
                                       : first_failure->stage + ": " +
                                             first_failure->error;
      if (!detail_summary.empty()) {
        message = result->success == 0U
                      ? detail_summary
                      : "Partial import failure: " + detail_summary;
      }
    }
  }
  return bills::android::jni::MakeResponse(
      false, "business.import_failed", std::move(message), std::move(data));
}

auto export_parse_bundle(const std::string& config_dir,
                         const std::string& records_dir,
                         const std::string& output_zip_path) -> std::string {
  if (config_dir.empty() || records_dir.empty() || output_zip_path.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "configDir, recordsDir, and outputZipPath must be non-empty.");
  }

  const auto result = bills::io::ExportParseBundle(records_dir, config_dir, output_zip_path);
  if (!result) {
    Json data;
    data["config_dir"] = config_dir;
    data["records_dir"] = records_dir;
    data["output_zip_path"] = output_zip_path;
    return bills::android::jni::MakeResponse(
        false, "business.export_failed", FormatError(result.error()),
        std::move(data));
  }

  Json data;
  data["config_dir"] = config_dir;
  data["records_dir"] = records_dir;
  data["output_zip_path"] = output_zip_path;
  data["exported_record_files"] = result->exported_record_files;
  data["exported_config_files"] = result->exported_config_files;
  return bills::android::jni::MakeResponse(
      true, "ok", "Parse bundle export finished.", std::move(data));
}

auto import_parse_bundle(const std::string& bundle_zip_path,
                         const std::string& config_dir,
                         const std::string& records_dir,
                         const std::string& db_path) -> std::string {
  if (bundle_zip_path.empty() || config_dir.empty() || records_dir.empty() ||
      db_path.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "bundleZipPath, configDir, recordsDir, and dbPath must be non-empty.");
  }

  const auto result =
      bills::io::ImportParseBundle(bundle_zip_path, config_dir, records_dir, db_path);
  if (!result) {
    Json data;
    data["bundle_zip_path"] = bundle_zip_path;
    data["config_dir"] = config_dir;
    data["records_dir"] = records_dir;
    data["db_path"] = db_path;
    return bills::android::jni::MakeResponse(
        false, "business.import_parse_bundle_failed", FormatError(result.error()),
        std::move(data));
  }

  Json data;
  data["bundle_zip_path"] = bundle_zip_path;
  data["config_dir"] = config_dir;
  data["records_dir"] = records_dir;
  data["db_path"] = db_path;
  data["imported_bills"] = result->imported_bills;
  data["imported_record_files"] = result->imported_record_files;
  data["imported_config_files"] = result->imported_config_files;
  if (!result->failed_phase.empty()) {
    data["failed_phase"] = result->failed_phase;
  }
  if (result->config_validation.processed > 0U || !result->config_validation.files.empty()) {
    data["config_validation"] = json_for_config_report(result->config_validation);
  }
  if (result->record_validation.processed > 0U || !result->record_validation.files.empty()) {
    data["record_validation"] = json_for_batch_result(result->record_validation);
  }
  if (result->db_ingest.processed > 0U || !result->db_ingest.files.empty()) {
    data["db_ingest"] = json_for_batch_result(result->db_ingest);
  }
  return bills::android::jni::MakeResponse(
      result->ok, result->ok ? "ok" : "business.import_parse_bundle_failed",
      result->message.empty()
          ? (result->ok ? "Parse bundle import finished."
                        : "Parse bundle import failed.")
          : result->message,
      std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_WorkspaceNativeBindings_importRecordsToDatabaseNative(
    JNIEnv* env, jclass, jstring config_dir, jstring records_dir,
    jstring db_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return import_records_to_database(
        bills::android::jni::FromJString(env, config_dir),
        bills::android::jni::FromJString(env, records_dir),
        bills::android::jni::FromJString(env, db_path));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_WorkspaceNativeBindings_exportParseBundleNative(
    JNIEnv* env, jclass, jstring config_dir, jstring records_dir,
    jstring output_zip_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return export_parse_bundle(bills::android::jni::FromJString(env, config_dir),
                               bills::android::jni::FromJString(env, records_dir),
                               bills::android::jni::FromJString(env, output_zip_path));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_WorkspaceNativeBindings_importParseBundleNative(
    JNIEnv* env, jclass, jstring bundle_zip_path, jstring config_dir,
    jstring records_dir, jstring db_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return import_parse_bundle(
        bills::android::jni::FromJString(env, bundle_zip_path),
        bills::android::jni::FromJString(env, config_dir),
        bills::android::jni::FromJString(env, records_dir),
        bills::android::jni::FromJString(env, db_path));
  });
}
