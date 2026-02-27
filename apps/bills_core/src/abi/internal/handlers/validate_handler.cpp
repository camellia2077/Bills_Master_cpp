#include "abi/internal/abi_shared.hpp"

#include "billing/conversion/bills_processing_pipeline.hpp"

namespace bills::core::abi {

auto handle_validate_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "'params' must be a JSON object.");
  }

  const std::string input_path = params.value("input_path", "");
  if (input_path.empty()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "Validate requires non-empty 'params.input_path'.");
  }

  BillConfig validator_config{BillValidationRules{}};
  Config modifier_config{};
  const std::string config_error =
      read_and_validate_configs(params, validator_config, modifier_config);
  if (!config_error.empty()) {
    Json data;
    data["detail"] = config_error;
    return make_response(false, error_code::kParamInvalidConfig,
                         "Failed to load/validate configuration.",
                         std::move(data));
  }

  std::vector<fs::path> files;
  try {
    files = list_txt_files(fs::path(input_path));
  } catch (const std::exception& ex) {
    Json data;
    data["detail"] = ex.what();
    return make_response(false, error_code::kParamInvalidInputPath,
                         "Failed to enumerate input files.", std::move(data));
  }

  if (files.empty()) {
    Json data;
    data["input_path"] = input_path;
    return make_response(false, error_code::kBusinessNoInputFiles,
                         "No .txt files found under input_path.", std::move(data));
  }

  BillProcessingPipeline pipeline(validator_config, modifier_config);

  Json file_results = Json::array();
  std::size_t success = 0;
  std::size_t failure = 0;

  for (const auto& file : files) {
    Json item;
    item["path"] = file.string();
    try {
      const std::string content = read_text_file(file);
      const bool ok = pipeline.validate_content(content, file.string());
      item["ok"] = ok;
      if (ok) {
        ++success;
      } else {
        ++failure;
      }
    } catch (const std::exception& ex) {
      item["ok"] = false;
      item["error"] = ex.what();
      ++failure;
    }
    file_results.push_back(std::move(item));
  }

  Json data;
  data["input_path"] = input_path;
  data["processed"] = files.size();
  data["success"] = success;
  data["failure"] = failure;
  data["all_valid"] = (failure == 0U);
  data["files"] = std::move(file_results);

  if (failure == 0U) {
    return make_response(true, error_code::kOk,
                         "Validation completed successfully.", std::move(data));
  }
  return make_response(false, error_code::kBusinessValidationFailed,
                       "One or more files failed validation.", std::move(data));
}

}  // namespace bills::core::abi
