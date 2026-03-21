#include <jni.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "bills_io/host_flow_support.hpp"
#include "config/config_bundle_service.hpp"
#include "jni_common.hpp"
#include "record_template/record_template_service.hpp"

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

auto json_for_invalid_record_file(const InvalidPeriodFile& invalid_file) -> Json {
  Json item;
  item["path"] = invalid_file.path;
  item["error"] = invalid_file.error;
  return item;
}

auto make_record_template_failure_response(const RecordTemplateError& error,
                                           std::string_view fallback_message)
    -> std::string {
  std::string code = "system.native_failure";
  if (error.category == RecordTemplateErrorCategory::kRequest ||
      error.category == RecordTemplateErrorCategory::kConfig ||
      error.category == RecordTemplateErrorCategory::kInputData ||
      error.category == RecordTemplateErrorCategory::kOutputData) {
    code = "param.invalid_argument";
  }

  Json data;
  data["detail"] = FormatRecordTemplateError(error);
  return bills::android::jni::MakeResponse(
      false, std::move(code),
      error.message.empty() ? std::string(fallback_message) : error.message,
      std::move(data));
}

auto load_validated_config_bundle(const std::string& config_dir)
    -> bills::io::HostConfigContext {
  const auto result = bills::io::LoadValidatedConfigContext(config_dir);
  if (!result) {
    throw std::runtime_error(FormatError(result.error()));
  }
  return *result;
}

auto generate_record_template(const std::string& config_dir,
                              const std::string& iso_month) -> std::string {
  if (config_dir.empty() || iso_month.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument",
        "configDir and isoMonth must be non-empty.");
  }

  const auto [documents, validated] = load_validated_config_bundle(config_dir);
  (void)validated;
  const auto layout =
      RecordTemplateService::BuildOrderedTemplateLayout(documents.validator);
  if (!layout) {
    return make_record_template_failure_response(
        layout.error(), "Failed to build template layout.");
  }

  TemplateGenerationRequest request;
  request.period = iso_month;
  request.layout = *layout;
  const auto result = RecordTemplateService::GenerateTemplates(request);
  if (!result) {
    return make_record_template_failure_response(
        result.error(), "Failed to generate record template.");
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

  const auto [_documents, validated] = load_validated_config_bundle(config_dir);
  const auto source_documents =
      bills::io::LoadSourceDocuments(fs::path(input_path), ".txt");
  if (!source_documents) {
    Json data;
    data["input_path"] = input_path;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument", FormatError(source_documents.error()),
        std::move(data));
  }

  const auto preview = RecordTemplateService::ValidateRecordBatch(
      *source_documents, validated.runtime_config, input_path);
  if (!preview) {
    return make_record_template_failure_response(preview.error(),
                                                 "Failed to preview record batch.");
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

auto list_record_periods(const std::string& input_path) -> std::string {
  if (input_path.empty()) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument", "inputPath must be non-empty.");
  }

  const auto source_documents =
      bills::io::LoadSourceDocuments(fs::path(input_path), ".txt");
  if (!source_documents) {
    Json data;
    data["input_path"] = input_path;
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument", FormatError(source_documents.error()),
        std::move(data));
  }

  const auto result = RecordTemplateService::ListPeriods(*source_documents, input_path);
  if (!result) {
    return make_record_template_failure_response(result.error(),
                                                 "Failed to list record periods.");
  }

  Json data;
  data["input_path"] = result->input_path;
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
    return bills::android::jni::MakeResponse(true, "ok",
                                             "No record files found yet.",
                                             std::move(data));
  }
  if (result->invalid == 0U) {
    return bills::android::jni::MakeResponse(
        true, "ok", "Record periods listed successfully.", std::move(data));
  }
  return bills::android::jni::MakeResponse(
      false, "business.record_periods_incomplete",
      "One or more record files did not contain a valid period.",
      std::move(data));
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
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_listRecordPeriodsNative(
    JNIEnv* env, jclass, jstring input_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return list_record_periods(bills::android::jni::FromJString(env, input_path));
  });
}
