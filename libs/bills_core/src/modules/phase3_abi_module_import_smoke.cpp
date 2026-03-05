import bill.core.abi;

namespace {
using bills::core::modules::abi::Json;
namespace abi = bills::core::modules::abi;

[[maybe_unused]] auto kMakeResponse = &abi::make_response;
[[maybe_unused]] auto kHandleValidate = &abi::handle_validate_command;
[[maybe_unused]] auto kHandleConvert = &abi::handle_convert_command;
[[maybe_unused]] auto kHandleIngest = &abi::handle_ingest_command;
[[maybe_unused]] auto kHandleImport = &abi::handle_import_command;
[[maybe_unused]] auto kHandleQuery = &abi::handle_query_command;
[[maybe_unused]] constexpr const char* kAbiVersion = abi::constants::kAbiVersion;
[[maybe_unused]] constexpr const char* kErrorOk = abi::error_code::kOk;
[[maybe_unused]] Json kEmptyObject = Json::object();
}  // namespace
