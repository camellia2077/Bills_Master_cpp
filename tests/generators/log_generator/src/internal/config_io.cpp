#include "config_io.h"

#include <cstdint>
#include <string_view>
#include <utility>

#include <toml++/toml.hpp>

namespace {

auto load_optional_string_array(const toml::table& table, std::string_view key,
                                std::string_view context,
                                std::vector<std::string>& target,
                                std::string& error_message) -> bool {
  target.clear();
  if (const toml::node* raw_node = table.get(key)) {
    const toml::array* raw_array = raw_node->as_array();
    if (raw_array == nullptr) {
      error_message = std::string(context) + "." + std::string(key) +
                      " must be an array of strings.";
      return false;
    }

    for (const auto& item_node : *raw_array) {
      if (const auto item = item_node.value<std::string>()) {
        target.push_back(*item);
      } else {
        error_message = std::string(context) + "." + std::string(key) +
                        " must contain only strings.";
        return false;
      }
    }
  }

  return true;
}

auto read_required_string(const toml::table& table, std::string_view key,
                          std::string_view context, std::string& target,
                          std::string& error_message) -> bool {
  if (const auto value = table[key].value<std::string>()) {
    target = *value;
    return true;
  }

  error_message = std::string(context) + " is missing string field '" +
                  std::string(key) + "'.";
  return false;
}

auto read_required_double(const toml::table& table, std::string_view key,
                          std::string_view context, double& target,
                          std::string& error_message) -> bool {
  if (const auto value = table[key].value<double>()) {
    target = *value;
    return true;
  }
  if (const auto value = table[key].value<std::int64_t>()) {
    target = static_cast<double>(*value);
    return true;
  }

  error_message = std::string(context) + " is missing numeric field '" +
                  std::string(key) + "'.";
  return false;
}

auto load_detail_config(const toml::table& detail_table,
                        GeneratorDetailConfig& detail_config,
                        std::string& error_message) -> bool {
  constexpr std::string_view kContext = "detail config";
  return read_required_string(detail_table, "description", kContext,
                              detail_config.description, error_message) &&
         read_required_double(detail_table, "min_cost", kContext,
                              detail_config.min_cost, error_message) &&
         read_required_double(detail_table, "max_cost", kContext,
                              detail_config.max_cost, error_message);
}

auto load_category_config(const toml::table& category_table,
                          GeneratorCategoryConfig& category_config,
                          std::string& error_message) -> bool {
  constexpr std::string_view kContext = "category config";
  if (!read_required_string(category_table, "parent_category", kContext,
                            category_config.parent_category, error_message) ||
      !read_required_string(category_table, "sub_category", kContext,
                            category_config.sub_category, error_message)) {
    return false;
  }

  category_config.details.clear();
  if (const toml::node* details_node = category_table.get("details")) {
    const toml::array* details_array = details_node->as_array();
    if (details_array == nullptr) {
      error_message = "category '" + category_config.sub_category +
                      "' field 'details' must be an array.";
      return false;
    }

    for (const auto& detail_node : *details_array) {
      const toml::table* detail_table = detail_node.as_table();
      if (detail_table == nullptr) {
        error_message = "category '" + category_config.sub_category +
                        "' contains a non-table detail item.";
        return false;
      }

      GeneratorDetailConfig detail_config;
      if (!load_detail_config(*detail_table, detail_config, error_message)) {
        return false;
      }
      category_config.details.push_back(std::move(detail_config));
    }
  }

  return true;
}

auto load_comments(const toml::table& comment_options,
                   GeneratorConfigData& config_data,
                   std::string& error_message) -> bool {
  if (const toml::node* probability_node = comment_options.get("probability")) {
    if (const auto value = probability_node->value<double>()) {
      config_data.comment_probability = *value;
    } else if (const auto value = probability_node->value<std::int64_t>()) {
      config_data.comment_probability = static_cast<double>(*value);
    } else {
      error_message = "comment_options.probability must be numeric.";
      return false;
    }
  } else {
    config_data.comment_probability = 0.3;
  }

  if (!load_optional_string_array(comment_options, "comments", "comment_options",
                                  config_data.comments, error_message)) {
    return false;
  }

  return true;
}

auto load_remark_options(const toml::table& remark_options,
                         GeneratorConfigData& config_data,
                         std::string& error_message) -> bool {
  return load_optional_string_array(remark_options, "summary_lines",
                                    "remark_options",
                                    config_data.remark_summary_lines,
                                    error_message) &&
         load_optional_string_array(remark_options, "followup_lines",
                                    "remark_options",
                                    config_data.remark_followup_lines,
                                    error_message);
}

}  // namespace

auto load_generator_config(const std::string& config_path,
                           GeneratorConfigData& config_data,
                           std::string& error_message) -> bool {
  try {
    config_data = GeneratorConfigData{};

    const toml::table raw_config = toml::parse_file(config_path);
    const toml::array* categories_array = raw_config["categories"].as_array();
    if (categories_array == nullptr) {
      error_message = "Config must contain a 'categories' array.";
      return false;
    }

    for (const auto& category_node : *categories_array) {
      const toml::table* category_table = category_node.as_table();
      if (category_table == nullptr) {
        error_message = "Config 'categories' must contain only tables.";
        return false;
      }

      GeneratorCategoryConfig category_config;
      if (!load_category_config(*category_table, category_config, error_message)) {
        return false;
      }
      config_data.categories.push_back(std::move(category_config));
    }

    if (const toml::table* comment_options =
            raw_config["comment_options"].as_table()) {
      if (!load_comments(*comment_options, config_data, error_message)) {
        return false;
      }
    }
    if (const toml::table* remark_options =
            raw_config["remark_options"].as_table()) {
      if (!load_remark_options(*remark_options, config_data, error_message)) {
        return false;
      }
    }
  } catch (const toml::parse_error& error) {
    error_message =
        "Failed to parse " + config_path + ". " + std::string(error.what());
    return false;
  } catch (const std::exception& error) {
    error_message =
        "Failed to load " + config_path + ". " + std::string(error.what());
    return false;
  }

  return true;
}
