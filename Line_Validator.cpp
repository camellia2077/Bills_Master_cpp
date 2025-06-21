#include "Line_Validator.h"
#include <regex>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
//#include "json.hpp" // 如果要使用json.hpp文件，则可以用这个
#include <nlohmann/json.hpp> // 确保你在编译环境中安装

// 使用 nlohmann::json 命名空间
using json = nlohmann::json;

namespace {
    const std::regex dateRegex(R"(^DATE:(\d{6})$)");
    const std::regex remarkRegex(R"(^REMARK:(.*)$)");
    const std::regex parentRegex(R"(^([A-Z].*)$)");
    const std::regex childRegex(R"(^([a-z_]+)$)");
    const std::regex itemRegex(R"(^(\d+(?:\.\d+)?)\s*(.*)$)");
}

// 构造函数，调用 load_config
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

bool LineValidator::is_valid_child_for_parent(const std::string& parent, const std::string& child) const {
    auto it = m_category_rules.find(parent);
    if (it == m_category_rules.end()) {
        // 如果父类别本身在配置中未定义，则任何子类别都无效
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
        result.type = "parent";
    } else {
        result.type = "unrecognized";
        result.matches.push_back(trimmedLine);
        return result;
    }
    
    for (size_t i = 1; i < matches.size(); ++i) {
        result.matches.push_back(matches[i].str());
    }

    return result;
}