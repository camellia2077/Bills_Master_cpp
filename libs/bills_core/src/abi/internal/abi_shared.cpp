#include "abi/internal/abi_shared.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

#include <toml++/toml.hpp>

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

auto parse_validator_config(const toml::table& validator_toml) -> BillConfig {
  BillValidationRules rules;
  if (const toml::array* categories = validator_toml["categories"].as_array();
      categories != nullptr) {
    for (const auto& category_node : *categories) {
      const toml::table* category = category_node.as_table();
      if (category == nullptr) {
        continue;
      }

      const auto* parent_title = category->get_as<std::string>("parent_item");
      if (parent_title == nullptr) {
        continue;
      }

      const std::string parent_title_value = parent_title->get();
      rules.parent_titles.insert(parent_title_value);

      std::set<std::string> sub_titles;
      if (const toml::array* sub_items =
              category->get_as<toml::array>("sub_items");
          sub_items != nullptr) {
        for (const auto& sub_item : *sub_items) {
          if (const auto* sub_title = sub_item.as_string()) {
            sub_titles.insert(sub_title->get());
          }
        }
      }

      rules.validation_map[parent_title_value] = std::move(sub_titles);
    }
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

auto read_double(const toml::node* node, double fallback) -> double {
  if (node == nullptr) {
    return fallback;
  }
  if (const auto* value = node->as_floating_point()) {
    return value->get();
  }
  if (const auto* value = node->as_integer()) {
    return static_cast<double>(value->get());
  }
  return fallback;
}

auto parse_modifier_config(const toml::table& modifier_toml) -> Config {
  Config config;

  if (const toml::table* renewal_config =
          modifier_toml["auto_renewal_rules"].as_table();
      renewal_config != nullptr) {
    if (const auto* enabled = renewal_config->get_as<bool>("enabled")) {
      config.auto_renewal.enabled = enabled->get();
    }

    if (config.auto_renewal.enabled) {
      if (const toml::array* rules = renewal_config->get_as<toml::array>("rules");
          rules != nullptr) {
        for (const auto& rule_node : *rules) {
          const toml::table* rule = rule_node.as_table();
          if (rule == nullptr) {
            continue;
          }

          const auto* header_location =
              rule->get_as<std::string>("header_location");
          const auto* description = rule->get_as<std::string>("description");
          config.auto_renewal.rules.push_back(
              {header_location != nullptr ? header_location->get() : "",
               read_double(rule->get("amount"), 0.0),
               description != nullptr ? description->get() : ""});
        }
      }
    }
  }

  if (const toml::array* metadata_prefixes =
          modifier_toml["metadata_prefixes"].as_array();
      metadata_prefixes != nullptr) {
    for (const auto& prefix_node : *metadata_prefixes) {
      if (const auto* prefix = prefix_node.as_string()) {
        config.metadata_prefixes.push_back(prefix->get());
      }
    }
  }

  if (const toml::table* display_name_maps =
          modifier_toml["display_name_maps"].as_table();
      display_name_maps != nullptr) {
    for (const auto& [map_key, map_node] : *display_name_maps) {
      const toml::table* lang_table = map_node.as_table();
      if (lang_table == nullptr) {
        continue;
      }

      std::map<std::string, std::string> lang_map;
      for (const auto& [lang_key, lang_node] : *lang_table) {
        if (const auto* lang_value = lang_node.as_string()) {
          lang_map[std::string(lang_key.str())] = lang_value->get();
        }
      }
      config.display_name_maps[std::string(map_key.str())] =
          std::move(lang_map);
    }
  }

  return config;
}

auto read_toml_file(const fs::path& file_path) -> toml::table {
  if (!fs::is_regular_file(file_path)) {
    throw std::runtime_error("config file does not exist: " +
                             file_path.string());
  }
  return toml::parse_file(file_path.string());
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
  if (params.contains("validator_config") && params.contains("modifier_config")) {
    try {
      validator_config = parse_validator_config(params.at("validator_config"));
      modifier_config = parse_modifier_config(params.at("modifier_config"));
    } catch (const std::exception& ex) {
      return std::string("Failed to convert inline configs: ") + ex.what();
    }
    return {};
  }

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

  toml::table validator_toml;
  toml::table modifier_toml;
  try {
    validator_toml = read_toml_file(validator_path);
    modifier_toml = read_toml_file(modifier_path);
  } catch (const std::exception& ex) {
    return ex.what();
  }

  std::string error_message;
  if (!ValidatorConfigValidator::validate(validator_toml, error_message)) {
    return "Validator config invalid: " + error_message;
  }
  if (!ModifierConfigValidator::validate(modifier_toml, error_message)) {
    return "Modifier config invalid: " + error_message;
  }

  try {
    validator_config = parse_validator_config(validator_toml);
    modifier_config = parse_modifier_config(modifier_toml);
  } catch (const std::exception& ex) {
    return std::string("Failed to convert configs: ") + ex.what();
  }
  return {};
}

}  // namespace bills::core::abi
