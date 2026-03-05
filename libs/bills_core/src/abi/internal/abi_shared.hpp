#ifndef ABI_INTERNAL_ABI_SHARED_HPP
#define ABI_INTERNAL_ABI_SHARED_HPP

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "billing/conversion/bills_processing_pipeline.hpp"
#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"
#include "billing/conversion/validator/config/bills_config.hpp"
#include "nlohmann/json.hpp"
#include "reports/standard_json/standard_report_assembler.hpp"
#include "reports/standard_json/standard_report_json_serializer.hpp"
#include "serialization/bills_json_serializer.hpp"

namespace bills::core::abi {

using Json = nlohmann::json;
namespace fs = std::filesystem;

namespace constants {
inline constexpr const char* kAbiVersion = "1.0.0";
inline constexpr const char* kValidatorConfigName = "Validator_Config.json";
inline constexpr const char* kModifierConfigName = "Modifier_Config.json";
inline constexpr const char* kDefaultConvertOutputDir = "output/txt2josn";
inline constexpr int kResponseSchemaVersion = 2;
inline constexpr int kCapabilitiesSchemaVersion = 1;
inline constexpr int kErrorCodeSchemaVersion = 1;
}  // namespace constants

namespace error_code {
inline constexpr const char* kOk = "ok";

inline constexpr const char* kParamInvalidArgument = "param.invalid_argument";
inline constexpr const char* kParamInvalidJson = "param.invalid_json";
inline constexpr const char* kParamInvalidRequest = "param.invalid_request";
inline constexpr const char* kParamUnknownCommand = "param.unknown_command";
inline constexpr const char* kParamInvalidConfig = "param.invalid_config";
inline constexpr const char* kParamInvalidInputPath = "param.invalid_input_path";

inline constexpr const char* kBusinessNoInputFiles = "business.no_input_files";
inline constexpr const char* kBusinessValidationFailed =
    "business.validation_failed";
inline constexpr const char* kBusinessConvertFailed = "business.convert_failed";
inline constexpr const char* kBusinessIngestFailed = "business.ingest_failed";
inline constexpr const char* kBusinessImportFailed = "business.import_failed";
inline constexpr const char* kBusinessQueryFailed = "business.query_failed";
inline constexpr const char* kBusinessQueryNotFound = "business.query_not_found";

inline constexpr const char* kSystemNotImplemented = "system.not_implemented";
}  // namespace error_code

auto allocate_owned_string(const std::string& text) -> const char*;

auto make_response(bool ok, std::string code, std::string message,
                   Json data = Json::object()) -> std::string;

auto build_capabilities() -> Json;
auto make_capabilities_json() -> std::string;

auto read_text_file(const fs::path& file_path) -> std::string;
auto list_txt_files(const fs::path& input_path) -> std::vector<fs::path>;
auto list_json_files(const fs::path& input_path) -> std::vector<fs::path>;
auto build_convert_output_path(const fs::path& output_dir,
                               const fs::path& input_file) -> fs::path;

auto read_and_validate_configs(const Json& params, BillConfig& validator_config,
                               Config& modifier_config) -> std::string;

auto handle_validate_command(const Json& request) -> std::string;
auto handle_convert_command(const Json& request) -> std::string;
auto handle_ingest_command(const Json& request) -> std::string;
auto handle_import_command(const Json& request) -> std::string;
auto handle_query_command(const Json& request) -> std::string;

}  // namespace bills::core::abi

#endif  // ABI_INTERNAL_ABI_SHARED_HPP
