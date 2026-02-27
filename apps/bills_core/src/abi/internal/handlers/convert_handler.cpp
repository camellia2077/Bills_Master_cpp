#include "abi/internal/abi_shared.hpp"

#include "billing/conversion/bills_processing_pipeline.hpp"
#include "serialization/bills_json_serializer.hpp"

namespace bills::core::abi {

auto handle_convert_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "'params' must be a JSON object.");
  }

  const std::string input_path = params.value("input_path", "");
  if (input_path.empty()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "Convert requires non-empty 'params.input_path'.");
  }

  const std::string output_dir_raw =
      params.value("output_dir", std::string(constants::kDefaultConvertOutputDir));
  const bool write_files = params.value("write_files", true);
  const bool include_serialized_json =
      params.value("include_serialized_json", false);

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

  const fs::path output_dir = fs::path(output_dir_raw);
  BillProcessingPipeline pipeline(validator_config, modifier_config);

  Json file_results = Json::array();
  std::size_t success = 0;
  std::size_t failure = 0;

  for (const auto& file : files) {
    Json item;
    item["path"] = file.string();
    ParsedBill bill_data{};

    try {
      const std::string content = read_text_file(file);
      const bool converted = pipeline.convert_content(content, bill_data);
      item["ok"] = converted;
      if (!converted) {
        ++failure;
        file_results.push_back(std::move(item));
        continue;
      }

      if (write_files) {
        const fs::path final_output_path =
            build_convert_output_path(output_dir, file);
        BillJsonSerializer::write_to_file(bill_data, final_output_path.string());
        item["output_path"] = final_output_path.string();
      }

      if (include_serialized_json) {
        item["json"] = BillJsonSerializer::serialize(bill_data);
      }

      ++success;
    } catch (const std::exception& ex) {
      item["ok"] = false;
      item["error"] = ex.what();
      ++failure;
    }

    file_results.push_back(std::move(item));
  }

  Json data;
  data["input_path"] = input_path;
  data["output_dir"] = output_dir_raw;
  data["write_files"] = write_files;
  data["processed"] = files.size();
  data["success"] = success;
  data["failure"] = failure;
  data["all_converted"] = (failure == 0U);
  data["files"] = std::move(file_results);

  if (failure == 0U) {
    return make_response(true, error_code::kOk, "Convert completed successfully.",
                         std::move(data));
  }
  return make_response(false, error_code::kBusinessConvertFailed,
                       "One or more files failed conversion.", std::move(data));
}

}  // namespace bills::core::abi
