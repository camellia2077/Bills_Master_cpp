// modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>

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
            // --- 核心修改：将 contents 的类型也改为 ordered_json ---
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
                size_t pos = description.find("(auto-renewal)");
                if (pos != std::string::npos) {
                    source = "auto_renewal";
                    description.erase(pos);
                    description.erase(description.find_last_not_of(" \t") + 1);
                    content_node["description"] = description;
                }
                
                content_node["source"] = source;

                if (!comment.empty()) {
                    content_node["comment"] = comment;
                }
                
                // --- 核心修改：现在可以直接 push_back，不再需要 static_cast ---
                contents.push_back(content_node);
            }
            sub_items_obj[sub.title] = contents;
        }
        categories[parent.title] = sub_items_obj;
    }

    root["categories"] = categories;
    return root.dump(4);
}

// _parse_content_line 辅助函数保持不变
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

    size_t start_paren = full_description_part.find('(');
    size_t end_paren = full_description_part.find(')');

    if (start_paren != std::string::npos && end_paren != std::string::npos && start_paren < end_paren) {
        comment = full_description_part.substr(start_paren + 1, end_paren - start_paren - 1);
        description = full_description_part.substr(0, start_paren);
    } else {
        description = full_description_part;
        comment = "";
    }
    
    description.erase(description.find_last_not_of(" \t\n\r") + 1);
}