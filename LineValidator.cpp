#include "LineValidator.h"
#include <regex>
#include <algorithm> // 用于 std::distance
#include <cctype>    // 用于 std::isspace

// 静态的正则表达式定义已从 Bill_Parser 移至此处。
// 它们保持在文件作用域内，因为它们是常量且无状态的。
namespace {
    const std::regex dateRegex(R"(^DATE:(\d{6})$)");
    const std::regex remarkRegex(R"(^REMARK:(.*)$)");
    const std::regex parentRegex(R"(^([A-Z].*)$)");
    const std::regex childRegex(R"(^([a-z_]+)$)");
    const std::regex itemRegex(R"(^(\d+(?:\.\d+)?)\s*(.*)$)");
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
        result.matches.push_back(trimmedLine); // 保存原始行内容用于错误报告
        return result;
    }
    
    // 保存所有从匹配中捕获的子组
    for (size_t i = 1; i < matches.size(); ++i) {
        result.matches.push_back(matches[i].str());
    }

    return result;
}