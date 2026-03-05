module;
#include "abi/internal/abi_shared.hpp"

export module bill.core.abi;

export namespace bills::core::modules::abi {
using Json = ::bills::core::abi::Json;
namespace fs {
using path = ::std::filesystem::path;
}

using BillValidationRules = ::BillValidationRules;
using BillConfig = ::BillConfig;
using Config = ::Config;

namespace constants {
inline constexpr const char* kAbiVersion = ::bills::core::abi::constants::kAbiVersion;
inline constexpr const char* kValidatorConfigName =
    ::bills::core::abi::constants::kValidatorConfigName;
inline constexpr const char* kModifierConfigName =
    ::bills::core::abi::constants::kModifierConfigName;
inline constexpr const char* kDefaultConvertOutputDir =
    ::bills::core::abi::constants::kDefaultConvertOutputDir;
inline constexpr int kResponseSchemaVersion =
    ::bills::core::abi::constants::kResponseSchemaVersion;
inline constexpr int kCapabilitiesSchemaVersion =
    ::bills::core::abi::constants::kCapabilitiesSchemaVersion;
inline constexpr int kErrorCodeSchemaVersion =
    ::bills::core::abi::constants::kErrorCodeSchemaVersion;
}  // namespace constants

namespace error_code {
inline constexpr const char* kOk = ::bills::core::abi::error_code::kOk;
inline constexpr const char* kParamInvalidArgument =
    ::bills::core::abi::error_code::kParamInvalidArgument;
inline constexpr const char* kParamInvalidJson =
    ::bills::core::abi::error_code::kParamInvalidJson;
inline constexpr const char* kParamInvalidRequest =
    ::bills::core::abi::error_code::kParamInvalidRequest;
inline constexpr const char* kParamUnknownCommand =
    ::bills::core::abi::error_code::kParamUnknownCommand;
inline constexpr const char* kParamInvalidConfig =
    ::bills::core::abi::error_code::kParamInvalidConfig;
inline constexpr const char* kParamInvalidInputPath =
    ::bills::core::abi::error_code::kParamInvalidInputPath;
inline constexpr const char* kBusinessNoInputFiles =
    ::bills::core::abi::error_code::kBusinessNoInputFiles;
inline constexpr const char* kBusinessValidationFailed =
    ::bills::core::abi::error_code::kBusinessValidationFailed;
inline constexpr const char* kBusinessConvertFailed =
    ::bills::core::abi::error_code::kBusinessConvertFailed;
inline constexpr const char* kBusinessIngestFailed =
    ::bills::core::abi::error_code::kBusinessIngestFailed;
inline constexpr const char* kBusinessImportFailed =
    ::bills::core::abi::error_code::kBusinessImportFailed;
inline constexpr const char* kBusinessQueryFailed =
    ::bills::core::abi::error_code::kBusinessQueryFailed;
inline constexpr const char* kBusinessQueryNotFound =
    ::bills::core::abi::error_code::kBusinessQueryNotFound;
inline constexpr const char* kSystemNotImplemented =
    ::bills::core::abi::error_code::kSystemNotImplemented;
}  // namespace error_code

using ::bills::core::abi::allocate_owned_string;
using ::bills::core::abi::make_response;
using ::bills::core::abi::build_capabilities;
using ::bills::core::abi::make_capabilities_json;
using ::bills::core::abi::read_text_file;
using ::bills::core::abi::list_txt_files;
using ::bills::core::abi::list_json_files;
using ::bills::core::abi::build_convert_output_path;
using ::bills::core::abi::read_and_validate_configs;
using ::bills::core::abi::handle_validate_command;
using ::bills::core::abi::handle_convert_command;
using ::bills::core::abi::handle_ingest_command;
using ::bills::core::abi::handle_import_command;
using ::bills::core::abi::handle_query_command;
}  // namespace bills::core::modules::abi
