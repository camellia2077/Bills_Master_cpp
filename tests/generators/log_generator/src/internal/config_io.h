#ifndef CONFIG_IO_H
#define CONFIG_IO_H

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct GeneratorConfigData {
  nlohmann::json categories;
  double comment_probability = 0.0;
  std::vector<std::string> comments;
};

auto load_generator_config(const std::string& config_path,
                           GeneratorConfigData& config_data,
                           std::string& error_message) -> bool;

#endif  // CONFIG_IO_H
