#ifndef CONFIG_IO_H
#define CONFIG_IO_H

#include <string>
#include <vector>

struct GeneratorDetailConfig {
  std::string description;
  double min_cost = 0.0;
  double max_cost = 0.0;
};

struct GeneratorCategoryConfig {
  std::string parent_category;
  std::string sub_category;
  std::vector<GeneratorDetailConfig> details;
};

struct GeneratorConfigData {
  std::vector<GeneratorCategoryConfig> categories;
  double comment_probability = 0.0;
  std::vector<std::string> comments;
  std::vector<std::string> remark_summary_lines;
  std::vector<std::string> remark_followup_lines;
};

auto load_generator_config(const std::string& config_path,
                           GeneratorConfigData& config_data,
                           std::string& error_message) -> bool;

#endif  // CONFIG_IO_H
