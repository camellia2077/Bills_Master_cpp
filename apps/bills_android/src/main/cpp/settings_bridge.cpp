#include <jni.h>

#include <string>

#include "common/version.hpp"
#include "io/host_flow_support.hpp"
#include "jni_common.hpp"

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

auto core_version_info() -> std::string {
  Json data;
  data["version_name"] = std::string(bills::core::version::kVersion);
  data["last_updated"] = std::string(bills::core::version::kLastUpdated);
  return bills::android::jni::MakeResponse(
      true, "ok", "Core version loaded successfully.", std::move(data));
}

auto validate_config_texts(const std::string& validator_text,
                           const std::string& modifier_text,
                           const std::string& export_formats_text) -> std::string {
  const auto result = bills::io::ValidateConfigTexts(
      validator_text, modifier_text, export_formats_text);
  if (!result) {
    return bills::android::jni::MakeResponse(
        false, "business.validation_failed", FormatError(result.error()));
  }

  Json data;
  data["config_validation"] = json_for_config_report(result->config_validation);
  data["enabled_export_formats"] = result->enabled_export_formats;
  data["available_export_formats"] = result->available_export_formats;
  return bills::android::jni::MakeResponse(
      result->ok, result->ok ? "ok" : "business.validation_failed",
      result->message, std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_SettingsNativeBindings_coreVersionNative(
    JNIEnv* env, jclass) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return core_version_info();
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_SettingsNativeBindings_validateConfigTextsNative(
    JNIEnv* env, jclass, jstring validator_text, jstring modifier_text,
    jstring export_formats_text) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return validate_config_texts(bills::android::jni::FromJString(env, validator_text),
                                 bills::android::jni::FromJString(env, modifier_text),
                                 bills::android::jni::FromJString(env,
                                                                   export_formats_text));
  });
}
