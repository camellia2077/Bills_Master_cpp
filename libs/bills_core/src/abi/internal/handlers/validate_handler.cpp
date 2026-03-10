// abi/internal/handlers/validate_handler.cpp
#include <cstddef>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#if BILLS_CORE_MODULES_ENABLED
import bill.core.abi;
import bill.core.billing.pipeline;
namespace core_abi = bills::core::modules::abi;
using bills::core::modules::billing::BillProcessingPipeline;
#else
#include "abi/internal/abi_shared.hpp"
namespace core_abi = bills::core::abi;
#endif

namespace bills::core::abi {

#if BILLS_CORE_MODULES_ENABLED
using Json = core_abi::Json;
namespace fs = core_abi::fs;
using BillValidationRules = core_abi::BillValidationRules;
using BillConfig = core_abi::BillConfig;
using Config = core_abi::Config;
#endif

auto handle_validate_command(const Json& request) -> std::string {
  const Json kParams = request.value("params", Json::object());
  if (!kParams.is_object()) {
    return core_abi::make_response(false, core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  const std::string kInputPath = kParams.value("input_path", "");
  if (kInputPath.empty()) {
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        "Validate requires non-empty 'params.input_path'.");
  }

  BillConfig validator_config{BillValidationRules{}};
  Config modifier_config{};
  const std::string kConfigError =
      core_abi::read_and_validate_configs(kParams, validator_config, modifier_config);
  if (!kConfigError.empty()) {
    Json data;
    data["detail"] = kConfigError;
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidConfig,
        "Failed to load/validate configuration.", std::move(data));
  }

  std::vector<fs::path> files;
  try {
    files = core_abi::list_txt_files(fs::path(kInputPath));
  } catch (const std::exception& ex) {
    Json data;
    data["detail"] = ex.what();
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidInputPath,
        "Failed to enumerate input files.", std::move(data));
  }

  if (files.empty()) {
    Json data;
    data["input_path"] = kInputPath;
    return core_abi::make_response(
        false, core_abi::error_code::kBusinessNoInputFiles,
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
      const std::string kContent = core_abi::read_text_file(file);
      const bool kOk = pipeline.validate_content(kContent, file.string());
      item["ok"] = kOk;
      if (kOk) {
        ++success;
      } else {
        item["error"] = core_abi::format_pipeline_failure(
            pipeline, "validate_content", "Validation failed.");
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
  data["input_path"] = kInputPath;
  data["processed"] = files.size();
  data["success"] = success;
  data["failure"] = failure;
  data["all_valid"] = (failure == 0U);
  data["files"] = std::move(file_results);

  if (failure == 0U) {
    return core_abi::make_response(
        true, core_abi::error_code::kOk, "Validation completed successfully.",
        std::move(data));
  }
  return core_abi::make_response(
      false, core_abi::error_code::kBusinessValidationFailed,
      "One or more files failed validation.", std::move(data));
}

}  // namespace bills::core::abi
