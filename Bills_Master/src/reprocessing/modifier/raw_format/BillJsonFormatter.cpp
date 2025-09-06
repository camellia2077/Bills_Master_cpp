// reprocessing/modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>

// format 函数保持不变
std::string BillJsonFormatter::format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const {
    nlohmann::ordered_json root;

    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("DATE:", 0) == 0) {
            root["date"] = meta_line.substr(5);
        } else if (meta_line.rfind("REMARK:", 0) == 0) {
            root["remark"] = meta_line.substr(7);
        }
    }

    nlohmann::ordered_json categories = nlohmann::ordered_json::object();

    for (const auto& parent : bill_structure) {
        nlohmann::ordered_json sub_items_obj = nlohmann::ordered_json::object();
        for (const auto& sub : parent.sub_items) {
            nlohmann::ordered_json contents = nlohmann::ordered_json::array(); 
            
            for (const auto& content_line : sub.contents) {
                nlohmann::ordered_json content_node;
                double amount = 0.0;
                std::string description;
                std::string comment;

                _parse_content_line(content_line, amount, description, comment);
                
                content_node["description"] = description;
                content_node["amount"] = amount;

                std::string source = "manually_add";
                // 移除对 (auto-renewal) 的检查，因为它现在也应该被当作普通注释
                // size_t pos = description.find("(auto-renewal)"); 
                // if (pos != std::string::npos) {
                //     source = "auto_renewal";
                //     description.erase(pos);
                //     description.erase(description.find_last_not_of(" \t") + 1);
                //     content_node["description"] = description;
                // }
                
                content_node["source"] = source;

                if (!comment.empty()) {
                    content_node["comment"] = comment;
                }
                
                contents.push_back(content_node);
            }
            sub_items_obj[sub.title] = contents;
        }
        categories[parent.title] = sub_items_obj;
    }

    root["categories"] = categories;
    return root.dump(4);
}

// --- 核心修改：更新 _parse_content_line 以使用 // 作为分隔符 ---
void BillJsonFormatter::_parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const {
    std::smatch match;
    std::regex re(R"(^(\d+(?:\.\d+)?)\s*(.*))");
    std::string full_description_part;

    if (std::regex_match(line, match, re) && match.size() == 3) {
        try {
            amount = std::stod(match[1].str());
            full_description_part = match[2].str();
        } catch (const std::exception&) {
            amount = 0.0;
            full_description_part = line;
        }
    } else {
        amount = 0.0;
        full_description_part = line;
    }

    // --- 新的解析逻辑 ---
    size_t comment_pos = full_description_part.find("//");

    if (comment_pos != std::string::npos) {
        // 提取 // 后面的内容作为 comment
        comment = full_description_part.substr(comment_pos + 2);
        // 提取 // 前面的内容作为 description
        description = full_description_part.substr(0, comment_pos);
    } else {
        description = full_description_part;
        comment = ""; // 确保 comment 为空
    }
    
    // 清理 description 和 comment 两端的空白字符
    description.erase(description.find_last_not_of(" \t\n\r") + 1);
    comment.erase(0, comment.find_first_not_of(" \t\n\r"));
}