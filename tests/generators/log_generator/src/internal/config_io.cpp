#include "config_io.h"

#include <fstream>

auto load_generator_config(const std::string& config_path,
                           GeneratorConfigData& config_data,
                           std::string& error_message) -> bool {
  std::ifstream config_file(config_path);
  if (!config_file.is_open()) {
    error_message =
        "Could not find or open configuration file '" + config_path + "'.";
    return false;
  }

  try {
    nlohmann::json raw_config = nlohmann::json::parse(config_file);
    if (raw_config.is_array()) {
      config_data.categories = std::move(raw_config);
    } else if (raw_config.contains("categories") &&
               raw_config["categories"].is_array()) {
      config_data.categories = raw_config["categories"];
    } else {
      error_message = "Config root must be an array of sub-category objects.";
      return false;
    }

    if (raw_config.contains("comment_options")) {
      const auto& comment_options = raw_config["comment_options"];
      config_data.comment_probability = comment_options.value("probability", 0.3);
      if (comment_options.contains("comments") &&
          comment_options["comments"].is_array()) {
        config_data.comments =
            comment_options["comments"].get<std::vector<std::string>>();
      }
    }
  } catch (const nlohmann::json::parse_error& error) {
    error_message =
        "Failed to parse " + config_path + ". " + std::string(error.what());
    return false;
  }

  return true;
}
