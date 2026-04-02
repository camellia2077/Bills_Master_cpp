#include <jni.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
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


auto commit_record_document(const std::string& expected_period,
                            const std::string& raw_text,
                            const std::string& config_dir,
                            const std::string& records_root,
                            const std::string& db_path)
    -> std::string {
  if (expected_period.empty() || config_dir.empty() || records_root.empty() ||
      db_path.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "expectedPeriod, configDir, recordsRoot, and dbPath must be non-empty.");
  }

  const auto commit_result = bills::io::CommitRecordTextToWorkspaceAndDatabase(
      expected_period, raw_text, config_dir, records_root, db_path);

  Json data;
  data["config_dir"] = config_dir;
  data["records_root"] = records_root;
  data["db_path"] = db_path;
  data["expected_period"] = expected_period;
  data["period"] = commit_result.period;
  data["relative_path"] = commit_result.relative_path;
  data["overwritten"] = commit_result.overwritten;
  if (commit_result.ok) {
    data["text"] = raw_text;
    data["persisted"] = true;
  }
  if (!commit_result.error_message.empty()) {
    data["error_message"] = commit_result.error_message;
  }
  return bills::android::jni::MakeResponse(
      commit_result.ok, commit_result.ok ? "ok" : "business.record_commit_failed",
      commit_result.message, std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_generateRecordTemplateJsonNative(
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
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_commitRecordDocumentJsonNative(
    JNIEnv* env, jclass, jstring expected_period, jstring raw_text,
    jstring config_dir, jstring records_root, jstring db_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return commit_record_document(
        bills::android::jni::FromJString(env, expected_period),
        bills::android::jni::FromJString(env, raw_text),
        bills::android::jni::FromJString(env, config_dir),
        bills::android::jni::FromJString(env, records_root),
        bills::android::jni::FromJString(env, db_path));
  });
}
