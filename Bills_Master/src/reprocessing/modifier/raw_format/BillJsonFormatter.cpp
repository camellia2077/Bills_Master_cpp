// modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>

// --- 修改：使用 nlohmann::ordered_json 来保持键的插入顺序 ---
// 这可以确保 "date" 和 "remark" 出现在 "categories" 之前
std::string BillJsonFormatter::format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const {
    nlohmann::ordered_json root;

    // 1. 首先处理元数据 (DATE, REMARK 等)，将其先插入
    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("DATE:", 0) == 0) {
            root["date"] = meta_line.substr(5);
        } else if (meta_line.rfind("REMARK:", 0) == 0) {
            root["remark"] = meta_line.substr(7);
        }
    }

    // 2. 创建一个名为 "categories" 的 JSON 对象
    nlohmann::ordered_json categories = nlohmann::ordered_json::object();

    // 3. 遍历 C++ 的数据结构，构建 categories 对象
    for (const auto& parent : bill_structure) {
        nlohmann::ordered_json sub_items_obj = nlohmann::ordered_json::object();
        for (const auto& sub : parent.sub_items) {
            nlohmann::json contents = nlohmann::json::array(); // 内容数组不需要保持顺序
            for (const auto& content_line : sub.contents) {
                nlohmann::json content_node;
                double amount = 0.0;
                std::string description;

                _parse_content_line(content_line, amount, description);
                
                content_node["amount"] = amount;

                // --- 新增逻辑：检查是否为自动续费项 ---
                std::string source = "manually_add";
                size_t pos = description.find("(auto-renewal)");
                if (pos != std::string::npos) {
                    source = "auto_renewal";
                    // 从描述中移除 "(auto-renewal)" 标记和末尾的空格
                    description.erase(pos);
                    description.erase(description.find_last_not_of(" \t") + 1);
                }
                
                content_node["description"] = description;
                content_node["source"] = source; // 添加 source 字段
                contents.push_back(content_node);
            }
            sub_items_obj[sub.title] = contents;
        }
        categories[parent.title] = sub_items_obj;
    }

    // 4. 最后将 categories 对象插入
    root["categories"] = categories;

    // 5. 将构建好的 JSON 对象序列化为字符串
    return root.dump(4);
}

// 辅助函数：解析内容行 (此函数无需改动)
void BillJsonFormatter::_parse_content_line(const std::string& line, double& amount, std::string& description) const {
    std::smatch match;
    std::regex re(R"(^(\d+(?:\.\d+)?)\s*(.*))");
    if (std::regex_match(line, match, re) && match.size() == 3) {
        try {
            amount = std::stod(match[1].str());
            description = match[2].str();
        } catch (const std::exception&) {
            amount = 0.0;
            description = line;
        }
    } else {
        amount = 0.0;
        description = line;
    }
}