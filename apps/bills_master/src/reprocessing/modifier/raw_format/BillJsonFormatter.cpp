// reprocessing/modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>
#include <iomanip>
#include <sstream>

std::string BillJsonFormatter::format(
    const std::vector<ParentItem>& bill_structure, 
    const std::vector<std::string>& metadata_lines) const 
{
    nlohmann::ordered_json root;

    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("date:", 0) == 0) root["date"] = meta_line.substr(5);
        else if (meta_line.rfind("remark:", 0) == 0) root["remark"] = meta_line.substr(7);
    }

    nlohmann::ordered_json categories_obj = nlohmann::ordered_json::object();
    // --- 【核心修改 1】 ---
    // 引入三个变量来分别计算总收入、总支出和结余
    double total_income = 0.0;
    double total_expense = 0.0;
    // --- 修改结束 ---

    for (const auto& parent : bill_structure) {
        nlohmann::ordered_json parent_node;
        nlohmann::ordered_json transactions_array = nlohmann::ordered_json::array();
        double parent_sub_total = 0.0;

        for (const auto& sub : parent.sub_items) {
            for (const auto& content_line : sub.contents) {
                nlohmann::ordered_json transaction_node;
                double amount = 0.0;
                std::string description, comment;
                _parse_content_line(content_line, amount, description, comment);
                
                transaction_node["sub_category"] = sub.title;
                transaction_node["description"] = description;
                transaction_node["amount"] = amount;
                transaction_node["source"] = "manually_add";
                
                transaction_node["transaction_type"] = (amount >= 0) ? "Income" : "Expense";
                
                transaction_node["comment"] = comment.empty() ? nullptr : nlohmann::json(comment);
                
                transactions_array.push_back(transaction_node);
                parent_sub_total += amount;

                // --- 【核心修改 2】 ---
                // 根据金额的正负，累加到对应的总计中
                if (amount >= 0) {
                    total_income += amount;
                } else {
                    total_expense += amount;
                }
                // --- 修改结束 ---
            }
        }
        
        parent_node["display_name"] = parent.title;
        
        std::stringstream ss_sub_total;
        ss_sub_total << std::fixed << std::setprecision(2) << parent_sub_total;
        parent_node["sub_total"] = nlohmann::json::parse(ss_sub_total.str());
        parent_node["transactions"] = transactions_array;
        
        categories_obj[parent.title] = parent_node;
    }

    // --- 【核心修改 3】 ---
    // 移除旧的 total_amount，替换为三个新的总计字段
    double balance = total_income + total_expense;

    std::stringstream ss_total_income, ss_total_expense, ss_balance;
    ss_total_income << std::fixed << std::setprecision(2) << total_income;
    ss_total_expense << std::fixed << std::setprecision(2) << total_expense;
    ss_balance << std::fixed << std::setprecision(2) << balance;

    root["total_income"] = nlohmann::json::parse(ss_total_income.str());
    root["total_expense"] = nlohmann::json::parse(ss_total_expense.str());
    root["balance"] = nlohmann::json::parse(ss_balance.str());
    // --- 修改结束 ---

    root["categories"] = categories_obj;

    return root.dump(4);
}

void BillJsonFormatter::_parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const {
    std::smatch match;
    std::regex re(R"(^(-?\d+(?:\.\d+)?)\s*(.*))");
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