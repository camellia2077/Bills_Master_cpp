#include <jni.h>
#include <sqlite3.h>

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

auto import_sample(const std::string& input_path, const std::string& config_dir,
                   const std::string& db_path) -> std::string {
  if (input_path.empty() || config_dir.empty() || db_path.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "inputPath, configDir, and dbPath must be non-empty.");
  }
  if (!fs::exists(input_path)) {
    Json data;
    data["input_path"] = input_path;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "Bundled sample input path does not exist.", std::move(data));
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
  const auto result = bills::io::IngestDocuments(input_path, config_dir, db_path, false);
  if (!result) {
    Json data;
    data["input_path"] = input_path;
    data["config_dir"] = config_dir;
    data["db_path"] = db_path;
    data["database_reset"] = database_reset;
    return bills::android::jni::MakeResponse(
        false, "business.import_failed", FormatError(result.error()),
        std::move(data));
  }

  Json data;
  data["input_path"] = input_path;
  data["config_dir"] = config_dir;
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
        true, "ok", "Sample import finished.", std::move(data));
  }

  std::string message =
      result->success == 0U
          ? "Sample import failed before any bill was written."
          : "Sample import finished with partial failures.";
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

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_BundledSampleNativeBindings_importBundledSampleNative(
    JNIEnv* env, jclass, jstring input_path, jstring config_dir,
    jstring db_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return import_sample(bills::android::jni::FromJString(env, input_path),
                         bills::android::jni::FromJString(env, config_dir),
                         bills::android::jni::FromJString(env, db_path));
  });
}
