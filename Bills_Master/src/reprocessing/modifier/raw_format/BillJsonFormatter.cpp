// src/reprocessing/modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>

// --- 核心修改：重写 format 函数以生成下游模块期望的、按 parent_category 分组的结构 ---
std::string BillJsonFormatter::format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const {
    nlohmann::ordered_json root;

    // 1. 解析元数据 (日期, 备注)
    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("DATE:", 0) == 0) {
            root["date"] = meta_line.substr(5);
        } else if (meta_line.rfind("REMARK:", 0) == 0) {
            root["remark"] = meta_line.substr(7);
        }
    }

    nlohmann::ordered_json categories_obj = nlohmann::ordered_json::object();
    double total_amount = 0.0;

    // 2. 遍历父分类来创建分组
    for (const auto& parent : bill_structure) {
        nlohmann::ordered_json parent_node;
        nlohmann::ordered_json transactions_array = nlohmann::ordered_json::array();
        double parent_sub_total = 0.0;

        // 遍历所有子分类
        for (const auto& sub : parent.sub_items) {
            // 遍历该子分类下的所有交易内容行
            for (const auto& content_line : sub.contents) {
                nlohmann::ordered_json transaction_node;
                double amount = 0.0;
                std::string description;
                std::string comment;

                _parse_content_line(content_line, amount, description, comment);
                
                // 填充交易节点
                transaction_node["sub_category"] = sub.title;
                transaction_node["description"] = description;
                transaction_node["amount"] = amount;
                transaction_node["source"] = "manually_add";

                if (!comment.empty()) {
                    transaction_node["comment"] = comment;
                }
                
                // 将交易节点添加到数组中
                transactions_array.push_back(transaction_node);
                
                // 累加金额
                parent_sub_total += amount;
            }
        }

        // 将该父分类的小计和扁平化的交易列表存入节点
        parent_node["sub_total"] = parent_sub_total;
        parent_node["transactions"] = transactions_array; // <--- 这是解析器期望的键
        
        // 将父分类节点添加到 categories 对象中
        categories_obj[parent.title] = parent_node;

        // 累加总金额
        total_amount += parent_sub_total;
    }

    // 3. 将总金额和 categories 对象添加到根节点
    root["total_amount"] = total_amount;
    root["categories"] = categories_obj;

    return root.dump(4);
}

// _parse_content_line 函数保持不变
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

    size_t comment_pos = full_description_part.find("//");

    if (comment_pos != std::string::npos) {
        comment = full_description_part.substr(comment_pos + 2);
        description = full_description_part.substr(0, comment_pos);
    } else {
        description = full_description_part;
        comment = "";
    }
    
    description.erase(description.find_last_not_of(" \t\n\r") + 1);
    comment.erase(0, comment.find_first_not_of(" \t\n\r"));
}