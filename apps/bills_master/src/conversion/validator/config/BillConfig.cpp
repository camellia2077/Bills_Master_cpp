// conversion/validator/config/BillConfig.cpp

#include "BillConfig.hpp"

// 构造函数现在接收一个有效的json对象
BillConfig::BillConfig(const json& config_data) {
    _load_and_parse(config_data);
}

// _load_and_parse 被重命名和简化，不再抛出异常
void BillConfig::_load_and_parse(const json& config_data) {
    // 验证逻辑已被移除，这里直接解析
    for (const auto& category : config_data["categories"]) {
        std::string parent_title = category["parent_item"];
        all_parent_titles.insert(parent_title);

        std::set<std::string> sub_titles;
        if (category["sub_items"].is_array()) {
            for (const auto& sub : category["sub_items"]) {
                sub_titles.insert(sub.get<std::string>());
            }
        }
        validation_map[parent_title] = sub_titles;
    }
}

auto BillConfig::is_parent_title(const std::string& title) const -> bool {
  return all_parent_titles.contains(title);
}

auto BillConfig::is_valid_sub_title(const std::string& parent_title,
                                    const std::string& sub_title) const
    -> bool {
  if (!validation_map.contains(parent_title)) {
    return false;
  }
  const auto& sub_titles = validation_map.at(parent_title);
  return sub_titles.contains(sub_title);
}
