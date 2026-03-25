#include <jni.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "bills_io/host_flow_support.hpp"
#include "common/iso_period.hpp"
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

auto json_for_record_preview_file(const RecordPreviewFile& file) -> Json {
  Json item;
  item["path"] = file.path;
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
  item["issues"] = json_for_validation_issues(file.issues);
  return item;
}

auto generate_record_template(const std::string& config_dir,
                              const std::string& iso_month) -> std::string {
  if (config_dir.empty() || iso_month.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "configDir and isoMonth must be non-empty.");
  }

  bills::io::HostTemplateGenerationRequest request;
  request.period = iso_month;
  const auto result = bills::io::GenerateTemplatesFromConfig(config_dir, request);
  if (!result) {
    Json data;
    data["detail"] = FormatError(result.error());
    return bills::android::jni::MakeResponse(
        false, "system.native_failure", "Failed to generate record template.",
        std::move(data));
  }
  if (result->templates.size() != 1U) {
    return bills::android::jni::MakeResponse(
        false, "system.native_failure",
        "Expected a single generated record template.");
  }

  const auto& generated = result->templates.front();
  Json data;
  data["period"] = generated.period;
  data["relative_path"] = generated.relative_path;
  data["text"] = generated.text;
  data["persisted"] = false;
  return bills::android::jni::MakeResponse(
      true, "ok", "Record template generated successfully.", std::move(data));
}

auto preview_record_path(const std::string& input_path,
                         const std::string& config_dir) -> std::string {
  if (input_path.empty() || config_dir.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "inputPath and configDir must be non-empty.");
  }

  const auto preview = bills::io::PreviewRecordDocuments(input_path, config_dir);
  if (!preview) {
    Json data;
    data["input_path"] = input_path;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument", FormatError(preview.error()),
        std::move(data));
  }

  Json data;
  data["input_path"] = preview->input_path;
  data["processed"] = preview->processed;
  data["success"] = preview->success;
  data["failure"] = preview->failure;
  data["all_valid"] = preview->failure == 0U;
  data["periods"] = preview->periods;
  Json files = Json::array();
  for (const auto& file : preview->files) {
    files.push_back(json_for_record_preview_file(file));
  }
  data["files"] = std::move(files);
  if (preview->failure == 0U) {
    return bills::android::jni::MakeResponse(
        true, "ok", "Record preview completed successfully.", std::move(data));
  }
  return bills::android::jni::MakeResponse(
      false, "business.validation_failed",
      "One or more files failed preview.", std::move(data));
}

auto is_missing_bills_table_error(std::string_view message) -> bool {
  return message.find("no such table: bills") != std::string_view::npos;
}

auto extract_period_from_document_text(std::string_view text)
    -> std::optional<std::string> {
  const std::size_t line_end = text.find('\n');
  std::string first_line =
      std::string(text.substr(0U, line_end == std::string_view::npos
                                      ? text.size()
                                      : line_end));
  if (!first_line.empty() && first_line.back() == '\r') {
    first_line.pop_back();
  }
  return bills::core::common::iso_period::extract_year_month_from_date_header(
      first_line);
}

auto list_database_record_periods(const std::string& db_path) -> std::string {
  if (db_path.empty()) {
    return bills::android::jni::MakeResponse(false, "param.invalid_argument",
                                             "dbPath must be non-empty.");
  }

  Json data;
  data["db_path"] = db_path;
  data["periods"] = Json::array();
  data["count"] = 0;

  if (!fs::exists(fs::path(db_path))) {
    return bills::android::jni::MakeResponse(
        true, "ok", "No imported months found in database.", std::move(data));
  }

  try {
    const auto periods = bills::io::ListAvailableMonths(db_path);
    if (!periods) {
      throw std::runtime_error(FormatError(periods.error()));
    }
    data["periods"] = *periods;
    data["count"] = periods->size();
    return bills::android::jni::MakeResponse(
        true, "ok",
        periods->empty() ? "No imported months found in database."
                        : "Database record periods loaded successfully.",
        std::move(data));
  } catch (const std::exception& error) {
    if (is_missing_bills_table_error(error.what())) {
      return bills::android::jni::MakeResponse(
          true, "ok", "No imported months found in database.", std::move(data));
    }
    throw;
  }
}

auto sync_saved_record_to_database(const std::string& input_path,
                                   const std::string& config_dir,
                                   const std::string& db_path,
                                   const std::string& expected_period)
    -> std::string {
  if (input_path.empty() || config_dir.empty() || db_path.empty() ||
      expected_period.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "inputPath, configDir, dbPath, and expectedPeriod must be non-empty.");
  }
  if (!fs::exists(fs::path(input_path)) || !fs::is_regular_file(fs::path(input_path))) {
    Json data;
    data["input_path"] = input_path;
    data["expected_period"] = expected_period;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "Saved TXT file does not exist.", std::move(data));
  }

  const auto source_documents =
      bills::io::LoadSourceDocuments(fs::path(input_path), ".txt");
  if (!source_documents) {
    Json data;
    data["input_path"] = input_path;
    data["expected_period"] = expected_period;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument", FormatError(source_documents.error()),
        std::move(data));
  }
  if (source_documents->size() != 1U) {
    Json data;
    data["input_path"] = input_path;
    data["expected_period"] = expected_period;
    return bills::android::jni::MakeResponse(
        false, "system.native_failure",
        "Expected a single TXT file for database sync.", std::move(data));
  }

  const auto actual_period =
      extract_period_from_document_text(source_documents->front().text);
  if (!actual_period.has_value()) {
    Json data;
    data["input_path"] = input_path;
    data["expected_period"] = expected_period;
    data["error_message"] = "The first line must be 'date:YYYY-MM'.";
    return bills::android::jni::MakeResponse(
        false, "business.validation_failed",
        "The first line must be 'date:YYYY-MM'.", std::move(data));
  }
  if (*actual_period != expected_period) {
    const std::string error_message = "TXT header period '" + *actual_period +
                                      "' does not match selected period '" +
                                      expected_period + "'.";
    Json data;
    data["input_path"] = input_path;
    data["expected_period"] = expected_period;
    data["actual_period"] = *actual_period;
    data["error_message"] = error_message;
    return bills::android::jni::MakeResponse(
        false, "business.period_mismatch", error_message, std::move(data));
  }

  std::error_code create_error;
  fs::create_directories(fs::path(db_path).parent_path(), create_error);
  if (create_error) {
    Json data;
    data["db_path"] = db_path;
    data["error_message"] = "Failed to prepare the database directory.";
    return bills::android::jni::MakeResponse(
        false, "system.io_failure",
        "Failed to prepare the database directory.", std::move(data));
  }

  const auto result = bills::io::IngestDocuments(input_path, config_dir, db_path, false);
  if (!result) {
    Json data;
    data["input_path"] = input_path;
    data["config_dir"] = config_dir;
    data["db_path"] = db_path;
    data["expected_period"] = expected_period;
    data["actual_period"] = *actual_period;
    data["error_message"] = FormatError(result.error());
    return bills::android::jni::MakeResponse(
        false, "business.editor_sync_failed", FormatError(result.error()),
        std::move(data));
  }

  Json data;
  data["input_path"] = input_path;
  data["config_dir"] = config_dir;
  data["db_path"] = db_path;
  data["expected_period"] = expected_period;
  data["actual_period"] = *actual_period;
  data["processed"] = result->processed;
  data["success"] = result->success;
  data["failure"] = result->failure;

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

  if (result->failure == 0U) {
    return bills::android::jni::MakeResponse(
        true, "ok", "TXT saved and synced to database successfully.",
        std::move(data));
  }

  std::string error_message = "Failed to sync the saved TXT into the database.";
  if (!result->files.empty()) {
    const auto first_failure =
        std::find_if(result->files.begin(), result->files.end(),
                     [](const BillWorkflowFileResult& file) { return !file.ok; });
    if (first_failure != result->files.end()) {
      error_message = first_failure->stage.empty()
                          ? first_failure->error
                          : first_failure->stage + ": " + first_failure->error;
    }
  }
  data["error_message"] = error_message;
  if (!failures.empty()) {
    data["first_failure"] = failures.front();
    data["failures"] = std::move(failures);
  }
  return bills::android::jni::MakeResponse(
      false, "business.editor_sync_failed", error_message, std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_generateRecordTemplateNative(
    JNIEnv* env, jclass, jstring config_dir, jstring iso_month) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return generate_record_template(
        bills::android::jni::FromJString(env, config_dir),
        bills::android::jni::FromJString(env, iso_month));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_previewRecordPathNative(
    JNIEnv* env, jclass, jstring input_path, jstring config_dir) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return preview_record_path(bills::android::jni::FromJString(env, input_path),
                               bills::android::jni::FromJString(env, config_dir));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_listDatabaseRecordPeriodsNative(
    JNIEnv* env, jclass, jstring db_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return list_database_record_periods(
        bills::android::jni::FromJString(env, db_path));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_syncSavedRecordToDatabaseNative(
    JNIEnv* env, jclass, jstring input_path, jstring config_dir, jstring db_path,
    jstring expected_period) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return sync_saved_record_to_database(
        bills::android::jni::FromJString(env, input_path),
        bills::android::jni::FromJString(env, config_dir),
        bills::android::jni::FromJString(env, db_path),
        bills::android::jni::FromJString(env, expected_period));
  });
}
