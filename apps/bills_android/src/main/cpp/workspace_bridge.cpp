#include <jni.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "io/host_flow_support.hpp"
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

  const auto ingest_result =
      bills::io::IngestDocumentsToDatabase(records_dir, config_dir, db_path, true);
  if (!ingest_result) {
    Json data;
    data["config_dir"] = config_dir;
    data["records_dir"] = records_dir;
    data["db_path"] = db_path;
    return bills::android::jni::MakeResponse(
        false, "business.import_failed", FormatError(ingest_result.error()),
        std::move(data));
  }

  const auto& result = ingest_result->ingest;
  Json data;
  data["config_dir"] = config_dir;
  data["records_dir"] = records_dir;
  data["db_path"] = db_path;
  data["database_reset"] = ingest_result->database_reset;
  data["processed"] = result.processed;
  data["success"] = result.success;
  data["failure"] = result.failure;
  data["imported"] = result.success;

  Json failures = Json::array();
  for (const auto& file : result.files) {
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
  if (result.failure == 0U) {
    return bills::android::jni::MakeResponse(
        true, "ok", "Record import finished.", std::move(data));
  }

  std::string message =
      result.success == 0U
          ? "Record import failed before any bill was written."
          : "Record import finished with partial failures.";
  if (!result.files.empty()) {
    for (const auto& file : result.files) {
      if (file.ok) {
        continue;
      }
      const std::string detail_summary =
          file.stage.empty() ? file.error : file.stage + ": " + file.error;
      if (!detail_summary.empty()) {
        message = result.success == 0U
                      ? detail_summary
                      : "Partial import failure: " + detail_summary;
        break;
      }
    }
  }
  return bills::android::jni::MakeResponse(
      false, "business.import_failed", std::move(message), std::move(data));
}

auto import_txt_directory_to_records(const std::string& source_records_dir,
                                     const std::string& config_dir,
                                     const std::string& records_root) -> std::string {
  if (source_records_dir.empty() || config_dir.empty() || records_root.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "sourceRecordsDir, configDir, and recordsRoot must be non-empty.");
  }

  const auto result = bills::io::ImportRecordDirectoryToWorkspace(
      source_records_dir, config_dir, records_root);
  if (!result) {
    Json data;
    data["source_records_dir"] = source_records_dir;
    data["config_dir"] = config_dir;
    data["records_root"] = records_root;
    return bills::android::jni::MakeResponse(
        false, "system.native_failure", FormatError(result.error()),
        std::move(data));
  }

  Json data;
  data["source_records_dir"] = source_records_dir;
  data["config_dir"] = config_dir;
  data["records_root"] = records_root;
  data["processed"] = result->processed;
  data["imported"] = result->imported;
  data["overwritten"] = result->overwritten;
  data["failure"] = result->failure;
  data["invalid"] = result->invalid;
  data["duplicate_period_conflicts"] = result->duplicate_period_conflicts;
  if (!result->first_failure_message.empty()) {
    data["first_failure_message"] = result->first_failure_message;
  }

  const bool ok = result->failure == 0U;
  const std::string message =
      ok ? "TXT directory import finished."
         : (result->first_failure_message.empty()
                ? "TXT directory import finished with failures."
                : result->first_failure_message);
  return bills::android::jni::MakeResponse(
      ok, ok ? "ok" : "business.import_failed", message, std::move(data));
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
Java_com_billstracer_android_data_nativebridge_WorkspaceNativeBindings_importTxtDirectoryToRecordsNative(
    JNIEnv* env, jclass, jstring source_records_dir, jstring config_dir,
    jstring records_root) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return import_txt_directory_to_records(
        bills::android::jni::FromJString(env, source_records_dir),
        bills::android::jni::FromJString(env, config_dir),
        bills::android::jni::FromJString(env, records_root));
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
