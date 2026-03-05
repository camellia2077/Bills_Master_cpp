#include "abi/internal/abi_shared.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

#if BILLS_CORE_MODULES_ENABLED
import bill.core.config.modifier;
import bill.core.config.validator;
using bills::core::modules::config_validator::ModifierConfigValidator;
using bills::core::modules::config_validator::ValidatorConfigValidator;
#else
#include "config_validator/pipeline/modifier_config_validator.hpp"
#include "config_validator/pipeline/validator_config_validator.hpp"
#endif

namespace bills::core::abi {

namespace {

auto resolve_error_layer(const std::string& code) -> std::string_view {
  if (code == error_code::kOk) {
    return "none";
  }
  if (code.rfind("param.", 0U) == 0U) {
    return "param";
  }
  if (code.rfind("business.", 0U) == 0U) {
    return "business";
  }
  if (code.rfind("system.", 0U) == 0U) {
    return "system";
  }
  return "system";
}

auto list_files_by_extension(const fs::path& input_path,
                             std::string_view extension)
    -> std::vector<fs::path> {
  if (!fs::exists(input_path)) {
    throw std::runtime_error("input_path does not exist: " +
                             input_path.string());
  }

  std::vector<fs::path> files;
  if (fs::is_regular_file(input_path)) {
    if (input_path.extension() == extension) {
      files.push_back(input_path);
    } else {
      throw std::runtime_error("input_path must match extension '" +
                               std::string(extension) +
                               "' or be a directory: " + input_path.string());
    }
  } else if (fs::is_directory(input_path)) {
    for (const auto& entry : fs::recursive_directory_iterator(input_path)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const fs::path file = entry.path();
      if (file.extension() == extension) {
        files.push_back(file);
      }
    }
  } else {
    throw std::runtime_error("input_path must be a file or directory: " +
                             input_path.string());
  }

  std::sort(files.begin(), files.end());
  return files;
}

auto parse_validator_config(const Json& validator_json) -> BillConfig {
  BillValidationRules rules;
  for (const auto& category : validator_json.at("categories")) {
    const std::string parent_title =
        category.at("parent_item").get<std::string>();
    rules.parent_titles.insert(parent_title);

    std::set<std::string> sub_titles;
    const auto& sub_items = category.at("sub_items");
    if (sub_items.is_array()) {
      for (const auto& sub_item : sub_items) {
        sub_titles.insert(sub_item.get<std::string>());
      }
    }
    rules.validation_map[parent_title] = std::move(sub_titles);
  }
  return BillConfig(std::move(rules));
}

auto parse_modifier_config(const Json& modifier_json) -> Config {
  Config config;

  if (modifier_json.contains("auto_renewal_rules")) {
    const auto& renewal_config = modifier_json["auto_renewal_rules"];
    config.auto_renewal.enabled = renewal_config.value("enabled", false);

    if (config.auto_renewal.enabled && renewal_config.contains("rules") &&
        renewal_config["rules"].is_array()) {
      for (const auto& rule_json : renewal_config["rules"]) {
        config.auto_renewal.rules.push_back(
            {rule_json.value("header_location", ""),
             rule_json.value("amount", 0.0),
             rule_json.value("description", "")});
      }
    }
  }

  if (modifier_json.contains("metadata_prefixes") &&
      modifier_json["metadata_prefixes"].is_array()) {
    for (const auto& prefix_json : modifier_json["metadata_prefixes"]) {
      if (prefix_json.is_string()) {
        config.metadata_prefixes.push_back(prefix_json.get<std::string>());
      }
    }
  }

  if (modifier_json.contains("display_name_maps") &&
      modifier_json["display_name_maps"].is_object()) {
    for (auto map_it = modifier_json["display_name_maps"].begin();
         map_it != modifier_json["display_name_maps"].end(); ++map_it) {
      if (!map_it.value().is_object()) {
        continue;
      }
      std::map<std::string, std::string> lang_map;
      for (auto lang_it = map_it.value().begin();
           lang_it != map_it.value().end(); ++lang_it) {
        if (lang_it.value().is_string()) {
          lang_map[lang_it.key()] = lang_it.value().get<std::string>();
        }
      }
      config.display_name_maps[map_it.key()] = std::move(lang_map);
    }
  }

  return config;
}

}  // namespace

auto allocate_owned_string(const std::string& text) -> const char* {
  const std::size_t buffer_size = text.size() + 1U;
  auto* buffer = static_cast<char*>(std::malloc(buffer_size));
  if (buffer == nullptr) {
    return nullptr;
  }
  std::memcpy(buffer, text.c_str(), buffer_size);
  return buffer;
}

auto make_response(bool ok, std::string code, std::string message,
                   Json data) -> std::string {
  const std::string_view error_layer = resolve_error_layer(code);
  Json response;
  response["ok"] = ok;
  response["code"] = std::move(code);
  response["message"] = std::move(message);
  response["data"] = std::move(data);
  response["error_layer"] = error_layer;
  response["abi_version"] = constants::kAbiVersion;
  response["response_schema_version"] = constants::kResponseSchemaVersion;
  response["error_code_schema_version"] = constants::kErrorCodeSchemaVersion;
  return response.dump();
}

auto build_capabilities() -> Json {
  Json caps;
  caps["abi_version"] = constants::kAbiVersion;
  caps["capabilities_schema_version"] = constants::kCapabilitiesSchemaVersion;
  caps["response_schema_version"] = constants::kResponseSchemaVersion;
  caps["error_code_schema_version"] = constants::kErrorCodeSchemaVersion;
  caps["supported_commands"] = {
      "validate",
      "convert",
      "ingest",
      "import",
      "query",
      "capabilities",
      "ping",
      "version",
  };
  caps["wired_commands"] = caps["supported_commands"];
  caps["error_layers"] = {
      "none",
      "param",
      "business",
      "system",
  };
  caps["error_code_examples"] = {
      {"param",
       {
           error_code::kParamInvalidRequest,
           error_code::kParamInvalidConfig,
           error_code::kParamInvalidInputPath,
       }},
      {"business",
       {
           error_code::kBusinessValidationFailed,
           error_code::kBusinessConvertFailed,
           error_code::kBusinessIngestFailed,
           error_code::kBusinessImportFailed,
           error_code::kBusinessQueryFailed,
       }},
      {"system",
       {
           error_code::kSystemNotImplemented,
       }},
  };
  caps["notes"] = {
      "JSON in/out only",
      "Use bills_core_free_string to release owned responses",
      "Command 'ingest' currently writes to in-memory repository only",
      "Command 'import' currently writes to in-memory repository only",
      "Command 'query' currently reads from JSON files (not database)",
  };
  return caps;
}

auto make_capabilities_json() -> std::string {
  return build_capabilities().dump();
}

auto read_text_file(const fs::path& file_path) -> std::string {
  std::ifstream input(file_path);
  if (!input) {
    throw std::runtime_error("Failed to open file: " + file_path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto list_txt_files(const fs::path& input_path) -> std::vector<fs::path> {
  return list_files_by_extension(input_path, ".txt");
}

auto list_json_files(const fs::path& input_path) -> std::vector<fs::path> {
  return list_files_by_extension(input_path, ".json");
}

auto build_convert_output_path(const fs::path& output_dir,
                               const fs::path& input_file) -> fs::path {
  fs::path modified_path = input_file;
  modified_path.replace_extension(".json");
  const std::string stem = modified_path.stem().string();

  fs::path final_output_path;
  if (stem.length() >= 4U) {
    const std::string year = stem.substr(0U, 4U);
    const fs::path target_dir = output_dir / year;
    fs::create_directories(target_dir);
    final_output_path = target_dir / modified_path.filename();
  } else {
    fs::create_directories(output_dir);
    final_output_path = output_dir / modified_path.filename();
  }
  return final_output_path;
}

auto read_and_validate_configs(const Json& params, BillConfig& validator_config,
                               Config& modifier_config) -> std::string {
  Json validator_json;
  Json modifier_json;

  if (params.contains("validator_config") && params.contains("modifier_config")) {
    validator_json = params.at("validator_config");
    modifier_json = params.at("modifier_config");
  } else {
    fs::path validator_path;
    fs::path modifier_path;

    const std::string config_dir = params.value("config_dir", "");
    if (!config_dir.empty()) {
      validator_path = fs::path(config_dir) / constants::kValidatorConfigName;
      modifier_path = fs::path(config_dir) / constants::kModifierConfigName;
    } else {
      const std::string validator_config_path =
          params.value("validator_config_path", "");
      const std::string modifier_config_path =
          params.value("modifier_config_path", "");
      if (validator_config_path.empty() || modifier_config_path.empty()) {
        return "Provide either 'config_dir' or both "
               "'validator_config_path' and 'modifier_config_path'.";
      }
      validator_path = fs::path(validator_config_path);
      modifier_path = fs::path(modifier_config_path);
    }

    try {
      validator_json = Json::parse(read_text_file(validator_path));
      modifier_json = Json::parse(read_text_file(modifier_path));
    } catch (const std::exception& ex) {
      return ex.what();
    }
  }

  std::string error_message;
  if (!ValidatorConfigValidator::validate(validator_json, error_message)) {
    return "Validator config invalid: " + error_message;
  }
  if (!ModifierConfigValidator::validate(modifier_json, error_message)) {
    return "Modifier config invalid: " + error_message;
  }

  try {
    validator_config = parse_validator_config(validator_json);
    modifier_config = parse_modifier_config(modifier_json);
  } catch (const std::exception& ex) {
    return std::string("Failed to convert configs: ") + ex.what();
  }
  return {};
}

}  // namespace bills::core::abi
