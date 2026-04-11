#include <jni.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "io/host_flow_support.hpp"
#include "jni_common.hpp"
#include "record_template/record_editor_service.hpp"

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

auto json_for_record_editor_entry(const RecordEditorEntry& entry) -> Json {
  return Json{{"amount_expression", entry.amount_expression},
              {"description", entry.description},
              {"comment", entry.comment}};
}

auto json_for_record_editor_sub_section(const RecordEditorSubSection& sub_section)
    -> Json {
  Json entries = Json::array();
  for (const auto& entry : sub_section.entries) {
    entries.push_back(json_for_record_editor_entry(entry));
  }
  return Json{{"title", sub_section.title}, {"entries", std::move(entries)}};
}

auto json_for_record_editor_parent_section(
    const RecordEditorParentSection& parent_section) -> Json {
  Json sub_sections = Json::array();
  for (const auto& sub_section : parent_section.sub_sections) {
    sub_sections.push_back(json_for_record_editor_sub_section(sub_section));
  }
  return Json{{"title", parent_section.title},
              {"sub_sections", std::move(sub_sections)}};
}

auto json_for_record_editor_document(const RecordEditorDocument& document) -> Json {
  Json remark_lines = Json::array();
  for (const auto& remark_line : document.remark_lines) {
    remark_lines.push_back(remark_line);
  }
  Json sections = Json::array();
  for (const auto& section : document.sections) {
    sections.push_back(json_for_record_editor_parent_section(section));
  }
  return Json{{"date_line", document.date_line},
              {"remark_lines", std::move(remark_lines)},
              {"sections", std::move(sections)}};
}

auto parse_record_editor_document(const Json& value, RecordEditorDocument& document,
                                  std::string& error) -> bool {
  if (!value.is_object()) {
    error = "'document' must be an object.";
    return false;
  }

  document = RecordEditorDocument{};
  document.date_line = value.value("date_line", "");
  if (value.contains("remark_lines")) {
    if (!value["remark_lines"].is_array()) {
      error = "'remark_lines' must be an array.";
      return false;
    }
    for (const auto& line : value["remark_lines"]) {
      if (!line.is_string()) {
        error = "'remark_lines' items must be strings.";
        return false;
      }
      document.remark_lines.push_back(line.get<std::string>());
    }
  }

  const Json sections = value.value("sections", Json::array());
  if (!sections.is_array()) {
    error = "'sections' must be an array.";
    return false;
  }

  for (const auto& parent_value : sections) {
    if (!parent_value.is_object()) {
      error = "'sections' items must be objects.";
      return false;
    }
    RecordEditorParentSection parent_section;
    parent_section.title = parent_value.value("title", "");
    const Json sub_sections = parent_value.value("sub_sections", Json::array());
    if (!sub_sections.is_array()) {
      error = "'sub_sections' must be an array.";
      return false;
    }

    for (const auto& sub_value : sub_sections) {
      if (!sub_value.is_object()) {
        error = "'sub_sections' items must be objects.";
        return false;
      }
      RecordEditorSubSection sub_section;
      sub_section.title = sub_value.value("title", "");
      const Json entries = sub_value.value("entries", Json::array());
      if (!entries.is_array()) {
        error = "'entries' must be an array.";
        return false;
      }
      for (const auto& entry_value : entries) {
        if (!entry_value.is_object()) {
          error = "'entries' items must be objects.";
          return false;
        }
        RecordEditorEntry entry;
        entry.amount_expression = entry_value.value("amount_expression", "");
        entry.description = entry_value.value("description", "");
        entry.comment = entry_value.value("comment", "");
        sub_section.entries.push_back(std::move(entry));
      }
      parent_section.sub_sections.push_back(std::move(sub_section));
    }
    document.sections.push_back(std::move(parent_section));
  }
  return true;
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

auto parse_record_editor_document_json(const std::string& raw_text) -> std::string {
  const auto parsed = RecordEditorService::Parse(raw_text);
  if (!parsed) {
    Json data;
    data["line"] = parsed.error().line;
    return bills::android::jni::MakeResponse(
        false, "business.record_editor_parse_failed", parsed.error().message,
        std::move(data));
  }
  return bills::android::jni::MakeResponse(
      true, "ok", "Record editor document parsed successfully.",
      json_for_record_editor_document(*parsed));
}

auto serialize_record_editor_document_json(const std::string& document_json)
    -> std::string {
  Json payload;
  try {
    payload = Json::parse(document_json);
  } catch (const std::exception& error) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_json", error.what());
  }

  RecordEditorDocument document;
  std::string parse_error;
  if (!parse_record_editor_document(payload, document, parse_error)) {
    return bills::android::jni::MakeResponse(
        false, "param.invalid_argument", parse_error);
  }

  const auto serialized = RecordEditorService::Serialize(document);
  if (!serialized) {
    Json data;
    data["line"] = serialized.error().line;
    return bills::android::jni::MakeResponse(
        false, "business.record_editor_serialize_failed",
        serialized.error().message, std::move(data));
  }

  Json data;
  data["text"] = *serialized;
  return bills::android::jni::MakeResponse(
      true, "ok", "Record editor document serialized successfully.",
      std::move(data));
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
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_parseRecordEditorDocumentJsonNative(
    JNIEnv* env, jclass, jstring raw_text) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return parse_record_editor_document_json(
        bills::android::jni::FromJString(env, raw_text));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_EditorNativeBindings_serializeRecordEditorDocumentJsonNative(
    JNIEnv* env, jclass, jstring document_json) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return serialize_record_editor_document_json(
        bills::android::jni::FromJString(env, document_json));
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
