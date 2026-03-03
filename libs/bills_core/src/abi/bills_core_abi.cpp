#include "abi/bills_core_abi.h"

#include <cstdlib>

#include "abi/internal/abi_shared.hpp"

namespace {

using Json = bills::core::abi::Json;
namespace abi = bills::core::abi;

}  // namespace

const char* bills_core_get_abi_version() { return abi::constants::kAbiVersion; }

const char* bills_core_get_capabilities_json() {
  return abi::allocate_owned_string(abi::make_capabilities_json());
}

const char* bills_core_invoke_json(const char* request_json_utf8) {
  if (request_json_utf8 == nullptr) {
    return abi::allocate_owned_string(
        abi::make_response(false, abi::error_code::kParamInvalidArgument,
                           "request_json_utf8 must not be null."));
  }

  Json request;
  try {
    request = Json::parse(request_json_utf8);
  } catch (const std::exception& ex) {
    Json data;
    data["parse_error"] = ex.what();
    return abi::allocate_owned_string(
        abi::make_response(false, abi::error_code::kParamInvalidJson,
                           "Failed to parse JSON request.", std::move(data)));
  }

  if (!request.is_object()) {
    return abi::allocate_owned_string(
        abi::make_response(false, abi::error_code::kParamInvalidRequest,
                           "Request root must be a JSON object."));
  }

  const std::string command = request.value("command", "");
  if (command.empty()) {
    return abi::allocate_owned_string(
        abi::make_response(false, abi::error_code::kParamInvalidRequest,
                           "Request must contain non-empty string field 'command'."));
  }

  if (command == "version") {
    Json data;
    data["abi_version"] = abi::constants::kAbiVersion;
    data["response_schema_version"] = abi::constants::kResponseSchemaVersion;
    data["capabilities_schema_version"] =
        abi::constants::kCapabilitiesSchemaVersion;
    data["error_code_schema_version"] = abi::constants::kErrorCodeSchemaVersion;
    return abi::allocate_owned_string(
        abi::make_response(true, abi::error_code::kOk, "ABI version returned.",
                           std::move(data)));
  }

  if (command == "capabilities") {
    Json data;
    data["capabilities"] = abi::build_capabilities();
    return abi::allocate_owned_string(
        abi::make_response(true, abi::error_code::kOk, "Capabilities returned.",
                           std::move(data)));
  }

  if (command == "ping") {
    Json data;
    data["pong"] = true;
    data["abi_version"] = abi::constants::kAbiVersion;
    data["response_schema_version"] = abi::constants::kResponseSchemaVersion;
    data["echo"] = request.value("payload", Json::object());
    return abi::allocate_owned_string(
        abi::make_response(true, abi::error_code::kOk, "Ping handled.",
                           std::move(data)));
  }

  if (command == "validate") {
    return abi::allocate_owned_string(abi::handle_validate_command(request));
  }

  if (command == "convert") {
    return abi::allocate_owned_string(abi::handle_convert_command(request));
  }

  if (command == "ingest") {
    return abi::allocate_owned_string(abi::handle_ingest_command(request));
  }

  if (command == "import") {
    return abi::allocate_owned_string(abi::handle_import_command(request));
  }

  if (command == "query") {
    return abi::allocate_owned_string(abi::handle_query_command(request));
  }

  Json data;
  data["command"] = command;
  return abi::allocate_owned_string(
      abi::make_response(false, abi::error_code::kParamUnknownCommand,
                         "Command is not supported.", std::move(data)));
}

void bills_core_free_string(const char* owned_utf8_str) {
  std::free(const_cast<char*>(owned_utf8_str));
}
