// Line_Validator.cpp
#include "Line_Validator.h"
#include <regex>
#include <algorithm>
#include <cctype>
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
        m_category_rules = data.get<std::map<std::string, std::vector<std::string>>>();
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
    const auto& valid_children = it->second;
    return std::find(valid_children.begin(), valid_children.end(), child) != valid_children.end();
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

    std::smatch matches;
    ValidationResult result;

    if (std::regex_match(trimmedLine, matches, dateRegex)) {
        result.type = "date";
    } else if (std::regex_match(trimmedLine, matches, remarkRegex)) {
        result.type = "remark";
    } else if (std::regex_match(trimmedLine, matches, itemRegex)) {
        result.type = "item";
    } else if (std::regex_match(trimmedLine, matches, childRegex)) {
        result.type = "child";
    } else if (std::regex_match(trimmedLine, matches, parentRegex)) {
        // **********************MODIFIED BLOCK START**********************
        // 捕获到可能是父项的行后，立即进行校验
        const std::string& parent_candidate = matches[1].str();
        if (is_valid_parent(parent_candidate)) {
            // 如果父项在JSON配置中存在，则标记为有效的"parent"类型
            result.type = "parent";
        } else {
            // 如果不存在，则标记为新的错误类型"invalid_parent"
            result.type = "invalid_parent";
        }
        // **********************MODIFIED BLOCK END**********************
    } else {
        result.type = "unrecognized";
        result.matches.push_back(trimmedLine);
        return result;
    }
    
    // 将正则表达式捕获到的内容存入matches向量
    for (size_t i = 1; i < matches.size(); ++i) {
        result.matches.push_back(matches[i].str());
    }

    return result;
}