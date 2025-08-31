// modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>

std::string BillJsonFormatter::format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const {
    nlohmann::json root;

    // 1. 处理元数据 (DATE, REMARK 等)
    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("DATE:", 0) == 0) {
            root["date"] = meta_line.substr(5);
        } else if (meta_line.rfind("REMARK:", 0) == 0) {
            root["remark"] = meta_line.substr(7);
        }
    }

    // 2. 创建一个名为 "categories" 的 JSON 对象 (而不是数组)
    nlohmann::json categories = nlohmann::json::object();

    // 3. 遍历 C++ 的数据结构，用标题作为 Key 来构建 JSON 对象
    for (const auto& parent : bill_structure) {
        nlohmann::json sub_items_obj = nlohmann::json::object();
        for (const auto& sub : parent.sub_items) {
            nlohmann::json contents = nlohmann::json::array();
            for (const auto& content_line : sub.contents) {
                nlohmann::json content_node;
                double amount = 0.0;
                std::string description;

                // 解析每一行内容，分离金额和描述
                _parse_content_line(content_line, amount, description);
                
                content_node["amount"] = amount;
                content_node["description"] = description;
                contents.push_back(content_node);
            }
            // 使用子标题 (e.g., "meal_low") 作为 Key
            sub_items_obj[sub.title] = contents;
        }
        // 使用父标题 (e.g., "MEAL吃饭") 作为 Key
        categories[parent.title] = sub_items_obj;
    }

    root["categories"] = categories;

    // 4. 将构建好的 JSON 对象序列化为字符串，并美化格式 (4个空格缩进)
    return root.dump(4);
}

// 辅助函数：解析内容行
void BillJsonFormatter::_parse_content_line(const std::string& line, double& amount, std::string& description) const {
    std::smatch match;
    // 使用正则表达式匹配 "数字" 和 "后面的所有文本"
    std::regex re(R"(^(\d+(?:\.\d+)?)\s*(.*))");
    if (std::regex_match(line, match, re) && match.size() == 3) {
        try {
            amount = std::stod(match[1].str());
            description = match[2].str();
        } catch (const std::exception&) {
            // 如果转换失败 (虽然不太可能)，则将整行作为描述
            amount = 0.0;
            description = line;
        }
    } else {
        // 如果不匹配，也把整行作为描述
        amount = 0.0;
        description = line;
    }
}