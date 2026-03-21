#include <exception>
#include <string>
#include <string_view>
#include <utility>

#include "record_template/record_template_service.hpp"

#if BILLS_CORE_MODULES_ENABLED
import bill.core.abi;
namespace core_abi = bills::core::modules::abi;
#else
#include "abi/internal/abi_shared.hpp"
namespace core_abi = bills::core::abi;
#endif

namespace bills::core::abi {

#if BILLS_CORE_MODULES_ENABLED
using Json = core_abi::Json;
#endif

namespace {

auto MakeServiceErrorResponse(const RecordTemplateError& error,
                              std::string_view default_message) -> std::string {
  const char* error_code = core_abi::error_code::kSystemNotImplemented;
  if (error.category == RecordTemplateErrorCategory::kRequest) {
    error_code = core_abi::error_code::kParamInvalidRequest;
  } else if (error.category == RecordTemplateErrorCategory::kConfig) {
    error_code = core_abi::error_code::kParamInvalidConfig;
  } else if (error.category == RecordTemplateErrorCategory::kInputPath) {
    error_code = core_abi::error_code::kParamInvalidInputPath;
  } else if (error.category == RecordTemplateErrorCategory::kOutputPath) {
    error_code = core_abi::error_code::kParamInvalidOutputPath;
  }

  Json data;
  data["detail"] = FormatRecordTemplateError(error);
  return core_abi::make_response(false, error_code, std::string(default_message),
                                 std::move(data));
}

auto SerializeTemplateFile(const GeneratedTemplateFile& generated_template,
                           bool include_text) -> Json {
  Json item;
  item["period"] = generated_template.period;
  item["relative_path"] = generated_template.relative_path.generic_string();
  if (include_text) {
    item["text"] = generated_template.text;
  } else {
    item["output_path"] = generated_template.output_path.string();
  }
  return item;
}

auto SerializeOrderedCategory(const OrderedTemplateCategory& category) -> Json {
  Json item;
  item["parent_item"] = category.parent_item;
  item["description"] = category.description;
  item["sub_items"] = category.sub_items;
  return item;
}

auto BuildTemplateRequest(const Json& params) -> TemplateGenerationRequest {
  TemplateGenerationRequest request;
  request.period = params.value("period", "");
  request.start_period = params.value("start_period", "");
  request.end_period = params.value("end_period", "");
  request.start_year = params.value("start_year", "");
  request.end_year = params.value("end_year", "");
  request.config_dir = params.value("config_dir", "");
  request.validator_config_path = params.value("validator_config_path", "");
  request.write_files = params.value("write_files", false);
  request.output_dir = params.value("output_dir", "");
  return request;
}

}  // namespace

auto handle_template_generate_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return core_abi::make_response(false, core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  const auto template_result =
      RecordTemplateService::GenerateTemplates(BuildTemplateRequest(params));
  if (!template_result) {
    return MakeServiceErrorResponse(template_result.error(),
                                    "Failed to generate record templates.");
  }

  Json data;
  data["write_files"] = template_result->write_files;
  data["generated"] = template_result->templates.size();
  if (template_result->write_files) {
    data["output_dir"] = template_result->output_dir.string();
  }

  Json templates = Json::array();
  for (const auto& generated_template : template_result->templates) {
    templates.push_back(
        SerializeTemplateFile(generated_template, !template_result->write_files));
  }
  data["templates"] = std::move(templates);

  return core_abi::make_response(true, core_abi::error_code::kOk,
                                 "Templates generated successfully.",
                                 std::move(data));
}

auto handle_config_inspect_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return core_abi::make_response(false, core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  RecordTemplateResult<ConfigInspectResult> inspect_result =
      std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kRequest, "missing config"));
  const std::string config_dir = params.value("config_dir", "");
  if (!config_dir.empty()) {
    inspect_result = RecordTemplateService::InspectConfig(config_dir);
  } else {
    const std::string validator_config_path =
        params.value("validator_config_path", "");
    if (validator_config_path.empty()) {
      return core_abi::make_response(
          false, core_abi::error_code::kParamInvalidRequest,
          "config_inspect requires config_dir or validator_config_path.");
    }
    inspect_result =
        RecordTemplateService::InspectValidatorFile(validator_config_path);
  }

  if (!inspect_result) {
    return MakeServiceErrorResponse(inspect_result.error(),
                                    "Failed to inspect config.");
  }

  Json categories = Json::array();
  for (const auto& category : inspect_result->categories) {
    categories.push_back(SerializeOrderedCategory(category));
  }

  Json data;
  data["schema_version"] = inspect_result->schema_version;
  data["date_format"] = inspect_result->date_format;
  data["metadata_headers"] = inspect_result->metadata_headers;
  data["categories"] = std::move(categories);

  return core_abi::make_response(true, core_abi::error_code::kOk,
                                 "Config inspected successfully.",
                                 std::move(data));
}

auto handle_list_periods_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return core_abi::make_response(false, core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  const std::string input_path = params.value("input_path", "");
  if (input_path.empty()) {
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        "list_periods requires non-empty 'params.input_path'.");
  }

  const auto list_result = RecordTemplateService::ListPeriods(input_path);
  if (!list_result) {
    return MakeServiceErrorResponse(list_result.error(),
                                    "Failed to list periods.");
  }

  if (list_result->processed == 0U) {
    Json data;
    data["input_path"] = input_path;
    return core_abi::make_response(
        false, core_abi::error_code::kBusinessNoInputFiles,
        "No .txt files found under input_path.", std::move(data));
  }

  Json invalid_files = Json::array();
  for (const auto& invalid_file : list_result->invalid_files) {
    Json item;
    item["path"] = invalid_file.path.string();
    item["error"] = invalid_file.error;
    invalid_files.push_back(std::move(item));
  }

  Json data;
  data["input_path"] = list_result->input_path.string();
  data["processed"] = list_result->processed;
  data["valid"] = list_result->valid;
  data["invalid"] = list_result->invalid;
  data["periods"] = list_result->periods;
  data["invalid_files"] = std::move(invalid_files);

  if (list_result->invalid == 0U) {
    return core_abi::make_response(true, core_abi::error_code::kOk,
                                   "Periods listed successfully.",
                                   std::move(data));
  }
  return core_abi::make_response(false,
                                 core_abi::error_code::kBusinessValidationFailed,
                                 "One or more files did not contain a valid period.",
                                 std::move(data));
}

}  // namespace bills::core::abi
