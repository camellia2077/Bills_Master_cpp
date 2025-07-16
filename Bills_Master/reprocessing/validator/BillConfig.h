#ifndef BILL_CONFIG_H
#define BILL_CONFIG_H

#include <string>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>

// 使用 nlohmann::json 的命名空间
using json = nlohmann::json;

/**
 * @class BillConfig
 * @brief 负责加载、解析和提供对账单验证规则的访问。
 *
 * 此类封装了所有与 JSON 配置文件相关的逻辑，
 * 将配置的读取和验证规则的提供分离开来。
 */
class BillConfig {
public:
    /**
     * @brief 构造函数，加载并解析配置文件。
     * @param config_path JSON 配置文件的路径。
     * @throws std::runtime_error 如果配置文件加载或解析失败。
     */
    explicit BillConfig(const std::string& config_path);

    /**
     * @brief 检查一个字符串是否是配置文件中定义的父标题。
     * @param title 要检查的标题。
     * @return 如果是有效的父标题，则返回 true。
     */
    bool is_parent_title(const std::string& title) const;

    /**
     * @brief 检查一个子标题对于给定的父标题是否有效。
     * @param parent_title 当前的父标题。
     * @param sub_title 要检查的子标题。
     * @return 如果子标题对于父标题是有效的，则返回 true。
     */
    bool is_valid_sub_title(const std::string& parent_title, const std::string& sub_title) const;

private:
    // 存储父标题 -> 子标题集合的验证规则
    std::unordered_map<std::string, std::set<std::string>> validation_map;
    // 存储所有有效的父标题，用于快速查找
    std::set<std::string> all_parent_titles;

    /**
     * @brief 从文件加载并解析 JSON 配置。
     * @param config_path 配置文件的路径。
     */
    void _load_and_parse(const std::string& config_path);
};

#endif // BILL_CONFIG_H
