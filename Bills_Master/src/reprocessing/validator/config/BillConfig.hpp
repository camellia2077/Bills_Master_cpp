// reprocessing/validator/config/BillConfig.hpp
#ifndef BILL_CONFIG_HPP
#define BILL_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class BillConfig {
public:
    /**
     * @brief 构造函数，从一个有效的 JSON 对象加载配置。
     * @param config_data 包含配置的 nlohmann::json 对象。
     */
    explicit BillConfig(const json& config_data);

    bool is_parent_title(const std::string& title) const;
    bool is_valid_sub_title(const std::string& parent_title, const std::string& sub_title) const;

private:
    std::unordered_map<std::string, std::set<std::string>> validation_map;
    std::set<std::string> all_parent_titles;

    /**
     * @brief 从 JSON 对象解析配置数据。
     * @param config_data 包含配置的 nlohmann::json 对象。
     */
    void _load_and_parse(const json& config_data);
};

#endif // BILL_CONFIG_HPP