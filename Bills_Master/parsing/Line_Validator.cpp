// Line_Validator.cpp
#include "Line_Validator.h"
#include <regex>
#include <algorithm> // For std::all_of
#include <cctype> // For std::isdigit, std::islower, std::isupper
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
    const std::regex dateRegex(R"(^DATE:(\d{6})$)");
    const std::regex remarkRegex(R"(^REMARK:(.*)$)");
    // 父项目正则表达式保持不变，因为它能正确捕获"大写字母+汉字"或"仅大写字母"的格式
    const std::regex parentRegex(R"(^([A-Z].*)$)"); 
    const std::regex childRegex(R"(^([a-z_]+)$)");
    const std::regex itemRegex(R"(^(\d+(?:\.\d+)?)\s*(.*)$)");
}

LineValidator::LineValidator(const std::string& config_path) {
    load_config(config_path);
}

void LineValidator::load_config(const std::string& config_path) {
    std::ifstream f(config_path);
    if (!f.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + config_path);
    }
    try {
        json data = json::parse(f);
        m_category_rules = data.get<std::map<std::string, std::unordered_set<std::string>>>();
    } catch (json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + config_path + ": " + e.what());
    }
}

// 新增的父类别校验函数
bool LineValidator::is_valid_parent(const std::string& parent) const {
    return m_category_rules.count(parent) > 0;
}

bool LineValidator::is_valid_child_for_parent(const std::string& parent, const std::string& child) const {
    auto it = m_category_rules.find(parent);
    if (it == m_category_rules.end()) {
        return false;
    }
    // 使用 unordered_set::count，其时间复杂度为 O(1)
    return it->second.count(child) > 0;
}

std::string LineValidator::trim(const std::string& s) const {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    if (start != end) {
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));
    }
    return std::string(start, end == s.end() ? end : end + 1);
}

ValidationResult LineValidator::validate(const std::string& line) const {
    std::string trimmedLine = trim(line);

    if (trimmedLine.empty()) {
        return {"empty", {}};
    }

    ValidationResult result;

    // 方案 1: 使用 C++20 的 starts_with，性能极高
    if (trimmedLine.starts_with("DATE:")) {
        std::string payload = trimmedLine.substr(5); // 提取 "DATE:" 后面的部分
        // 校验是否为6位数字，以匹配原正则表达式 ^DATE:(\d{6})$
        if (payload.length() == 6 && std::all_of(payload.begin(), payload.end(), ::isdigit)) {
            result.type = "date";
            result.matches.push_back(payload);
        } else {
            result.type = "unrecognized";
            result.matches.push_back(trimmedLine);
        }
    } else if (trimmedLine.starts_with("REMARK:")) {
        result.type = "remark";
        // 提取 "REMARK:" 后面的所有内容，匹配原正则表达式 ^REMARK:(.*)$
        result.matches.push_back(trimmedLine.substr(7));
    }
    // 方案 2: 检查是否为 item (以数字开头)
    else if (std::isdigit(static_cast<unsigned char>(trimmedLine[0]))) {
        result.type = "item";
        size_t space_pos = trimmedLine.find(' ');
        if (space_pos != std::string::npos) {
            // 分割金额和描述
            result.matches.push_back(trimmedLine.substr(0, space_pos)); // 金额部分
            size_t desc_start = trimmedLine.find_first_not_of(" \t", space_pos); // 跳过所有分隔空格
            if (desc_start != std::string::npos) {
                 result.matches.push_back(trimmedLine.substr(desc_start)); // 描述部分
            } else {
                 result.matches.push_back(""); // 只有数字和空格，没有描述
            }
        } else {
            // 整行只有数字，没有描述
            result.matches.push_back(trimmedLine);
            result.matches.push_back(""); // 描述为空
        }
    }
    // 方案 3: 检查是否为 child (以小写字母开头)
    else if (std::islower(static_cast<unsigned char>(trimmedLine[0]))) {
        // 校验是否所有字符均为小写字母或下划线，以匹配 ^([a-z_]+)$
        bool is_valid_format = std::all_of(trimmedLine.begin(), trimmedLine.end(), [](unsigned char c){
            return std::islower(c) || c == '_';
        });
        
        if (is_valid_format) {
            result.type = "child";
            result.matches.push_back(trimmedLine);
        } else {
            result.type = "unrecognized";
            result.matches.push_back(trimmedLine);
        }
    }
    // 方案 4: 检查是否为 parent (以大写字母开头)
    else if (std::isupper(static_cast<unsigned char>(trimmedLine[0]))) {
        const std::string& parent_candidate = trimmedLine;
        // 复用已有的校验逻辑
        if (is_valid_parent(parent_candidate)) {
            result.type = "parent";
            result.matches.push_back(parent_candidate);
        } else {
            result.type = "invalid_parent";
            result.matches.push_back(parent_candidate);
        }
    }
    // 方案 5: 无法识别
    else {
        result.type = "unrecognized";
        result.matches.push_back(trimmedLine);
    }

    return result;
}