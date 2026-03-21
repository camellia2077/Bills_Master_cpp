// abi/internal/abi_shared.cpp
#include "abi/internal/abi_shared.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

#include "config_loading/runtime_config_loader.hpp"

namespace bills::core::abi {

namespace {

auto ResolveErrorLayer(const std::string& code) -> std::string_view {
  if (code == error_code::kOk) {
    return "none";
  }
  if (code.starts_with("param.")) {
    return "param";
  }
  if (code.starts_with("business.")) {
    return "business";
  }
  if (code.starts_with("system.")) {
    return "system";
  }
  return "system";
}

auto ListFilesByExtension(const fs::path& input_path,
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
      const fs::path kFile = entry.path();
      if (kFile.extension() == extension) {
        files.push_back(kFile);
      }
    }
  } else {
    throw std::runtime_error("input_path must be a file or directory: " +
                             input_path.string());
  }

  std::ranges::sort(files);
  return files;
}

auto ParseValidatorConfig(const Json& validator_json) -> BillConfig {
  BillValidationRules rules;
  for (const auto& category : validator_json.at("categories")) {
    const std::string kParentTitle =
        category.at("parent_item").get<std::string>();
    rules.parent_titles.insert(kParentTitle);

    std::set<std::string> sub_titles;
    const auto& sub_items = category.at("sub_items");
    if (sub_items.is_array()) {
      for (const auto& sub_item : sub_items) {
        sub_titles.insert(sub_item.get<std::string>());
      }
    }
    rules.validation_map[kParentTitle] = std::move(sub_titles);
  }
  return BillConfig(std::move(rules));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- JSON config mapping mirrors the external schema directly.
auto ParseModifierConfig(const Json& modifier_json) -> Config {
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
  const std::size_t kBufferSize = text.size() + 1U;
  auto* buffer = static_cast<char*>(std::malloc(kBufferSize));
  if (buffer == nullptr) {
    return nullptr;
  }
  std::memcpy(buffer, text.c_str(), kBufferSize);
  return buffer;
}

auto make_response(bool is_ok, std::string code, std::string message,
                   Json data) -> std::string {
  const std::string_view kErrorLayer = ResolveErrorLayer(code);
  Json response;
  response["ok"] = is_ok;
  response["code"] = std::move(code);
  response["message"] = std::move(message);
  response["data"] = std::move(data);
  response["error_layer"] = kErrorLayer;
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
      "template_generate",
      "record_preview",
      "validate_config_bundle",
      "validate_record_batch",
      "preflight_import",
      "config_inspect",
      "list_periods",
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
           error_code::kParamInvalidOutputPath,
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
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to open file: " + file_path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto format_pipeline_failure(const BillProcessingPipeline& pipeline,
                             std::string_view fallback_stage,
                             std::string_view fallback_message)
    -> std::string {
  const std::string stage = pipeline.last_failure_stage().empty()
                                ? std::string(fallback_stage)
                                : pipeline.last_failure_stage();
  const std::string message = pipeline.last_failure_message().empty()
                                  ? std::string(fallback_message)
                                  : pipeline.last_failure_message();
  if (stage.empty()) {
    return message;
  }
  if (message.empty()) {
    return stage;
  }
  return stage + ": " + message;
}

auto list_txt_files(const fs::path& input_path) -> std::vector<fs::path> {
  return ListFilesByExtension(input_path, ".txt");
}

auto list_json_files(const fs::path& input_path) -> std::vector<fs::path> {
  return ListFilesByExtension(input_path, ".json");
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- call sites consistently pass output_dir first, then input_file.
auto build_convert_output_path(const fs::path& output_dir,
                               const fs::path& input_file) -> fs::path {
  fs::path modified_path = input_file;
  modified_path.replace_extension(".json");
  const std::string kStem = modified_path.stem().string();

  fs::path final_output_path;
  if (kStem.length() >= 4U) {
    const std::string kYear = kStem.substr(0U, 4U);
    const fs::path kTargetDir = output_dir / kYear;
    fs::create_directories(kTargetDir);
    final_output_path = kTargetDir / modified_path.filename();
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
      validator_config = ParseValidatorConfig(params.at("validator_config"));
      modifier_config = ParseModifierConfig(params.at("modifier_config"));
    } catch (const std::exception& ex) {
      return std::string("Failed to convert inline configs: ") + ex.what();
    }
    return {};
  }

  fs::path validator_path;
  fs::path modifier_path;

  const std::string kConfigDir = params.value("config_dir", "");
  if (!kConfigDir.empty()) {
    validator_path = fs::path(kConfigDir) / constants::kValidatorConfigName;
    modifier_path = fs::path(kConfigDir) / constants::kModifierConfigName;
  } else {
    const std::string kValidatorConfigPath =
        params.value("validator_config_path", "");
    const std::string kModifierConfigPath =
        params.value("modifier_config_path", "");
    if (kValidatorConfigPath.empty() || kModifierConfigPath.empty()) {
      return "Provide either 'config_dir' or both "
             "'validator_config_path' and 'modifier_config_path'.";
    }
    validator_path = fs::path(kValidatorConfigPath);
    modifier_path = fs::path(kModifierConfigPath);
  }

  const auto runtime_config =
      RuntimeConfigLoader::LoadFromFiles(validator_path, modifier_path);
  if (!runtime_config) {
    return FormatError(runtime_config.error());
  }
  validator_config = runtime_config->validator_config;
  modifier_config = runtime_config->modifier_config;
  return {};
}

}  // namespace bills::core::abi
