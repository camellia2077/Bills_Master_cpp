#include <jni.h>
#include <algorithm>
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

  const auto ingest_result =
      bills::io::IngestDocumentsToDatabase(input_path, config_dir, db_path, true);
  if (!ingest_result) {
    Json data;
    data["input_path"] = input_path;
    data["config_dir"] = config_dir;
    data["db_path"] = db_path;
    return bills::android::jni::MakeResponse(
        false, "business.import_failed", FormatError(ingest_result.error()),
        std::move(data));
  }

  const auto& result = ingest_result->ingest;
  Json data;
  data["input_path"] = input_path;
  data["config_dir"] = config_dir;
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
        true, "ok", "Sample import finished.", std::move(data));
  }

  std::string message =
      result.success == 0U
          ? "Sample import failed before any bill was written."
          : "Sample import finished with partial failures.";
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
