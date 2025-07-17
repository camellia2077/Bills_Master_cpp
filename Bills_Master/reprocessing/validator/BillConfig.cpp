#include "BillConfig.h"

BillConfig::BillConfig(const std::string& config_path) {
    _load_and_parse(config_path);
}

void BillConfig::_load_and_parse(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error("错误: 无法打开配置文件 '" + config_path + "'");
    }

    json config_data;
    try {
        file >> config_data;
    } catch (json::parse_error& e) {
        throw std::runtime_error("错误: 解析 JSON 配置文件失败: " + std::string(e.what()));
    }

    if (!config_data.contains("categories") || !config_data["categories"].is_array() || config_data["categories"].empty()) {
        throw std::runtime_error("错误: 配置文件格式不正确或 'categories' 列表为空。");
    }

    // 转换 JSON 数据为内部使用的数据结构
    for (const auto& category : config_data["categories"]) {
        if (!category.contains("parent_item") || !category.contains("sub_items")) continue;

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

bool BillConfig::is_parent_title(const std::string& title) const {
    return all_parent_titles.count(title) > 0;
}

bool BillConfig::is_valid_sub_title(const std::string& parent_title, const std::string& sub_title) const {
    // 首先确保父标题存在
    if (validation_map.count(parent_title) == 0) {
        return false;
    }
    // 然后检查子标题是否在对应的集合中
    const auto& sub_titles = validation_map.at(parent_title);
    return sub_titles.count(sub_title) > 0;
}
