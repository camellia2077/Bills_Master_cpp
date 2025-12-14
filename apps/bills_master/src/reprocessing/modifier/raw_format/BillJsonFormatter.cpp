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

    // 1. 处理元数据 (date, remark 等)
    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("date:", 0) == 0) root["date"] = meta_line.substr(5);
        else if (meta_line.rfind("remark:", 0) == 0) root["remark"] = meta_line.substr(7);
    }

    nlohmann::ordered_json categories_obj = nlohmann::ordered_json::object();
    
    // 初始化总计变量
    double total_income = 0.0;
    double total_expense = 0.0;

    // 2. 遍历父分类构建 JSON 结构
    for (const auto& parent : bill_structure) {
        nlohmann::ordered_json parent_node;
        nlohmann::ordered_json transactions_array = nlohmann::ordered_json::array();
        double parent_sub_total = 0.0; // 当前父分类的小计

        for (const auto& sub : parent.sub_items) {
            for (const auto& content_line : sub.contents) {
                nlohmann::ordered_json transaction_node;
                double amount = 0.0;
                std::string description, comment;
                
                // 解析单行内容
                _parse_content_line(content_line, amount, description, comment);
                
                // 填充交易节点字段
                transaction_node["sub_category"] = sub.title;
                transaction_node["description"] = description;
                transaction_node["amount"] = amount;
                transaction_node["source"] = "manually_add";
                transaction_node["transaction_type"] = (amount >= 0) ? "Income" : "Expense";
                transaction_node["comment"] = comment.empty() ? nullptr : nlohmann::json(comment);
                
                transactions_array.push_back(transaction_node);
                
                // 累加父分类小计
                parent_sub_total += amount;

                // 累加全局总收入/总支出
                if (amount >= 0) {
                    total_income += amount;
                } else {
                    total_expense += amount;
                }
            }
        }
        
        // 设置父分类节点属性
        parent_node["display_name"] = parent.title;
        
        // 格式化小计为字符串，保留两位小数
        std::stringstream ss_sub_total;
        ss_sub_total << std::fixed << std::setprecision(2) << parent_sub_total;
        parent_node["sub_total"] = nlohmann::json::parse(ss_sub_total.str());
        parent_node["transactions"] = transactions_array;
        
        categories_obj[parent.title] = parent_node;
    }

    // 3. 计算并设置全局统计数据
    double balance = total_income + total_expense;

    std::stringstream ss_total_income, ss_total_expense, ss_balance;
    ss_total_income << std::fixed << std::setprecision(2) << total_income;
    ss_total_expense << std::fixed << std::setprecision(2) << total_expense;
    ss_balance << std::fixed << std::setprecision(2) << balance;

    root["total_income"] = nlohmann::json::parse(ss_total_income.str());
    root["total_expense"] = nlohmann::json::parse(ss_total_expense.str());
    root["balance"] = nlohmann::json::parse(ss_balance.str());

    root["categories"] = categories_obj;

    return root.dump(4);
}

void BillJsonFormatter::_parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const {
    std::smatch match;

    // --- 正则表达式解析 ---
    // R"(...)" 是 C++ 的原始字符串字面量，方便书写正则
    // 解析计算后的金额与项目名称,比如"-60.50 肯德基"
    // ^                   : 匹配行首
    // (                   : 第 1 捕获组开始（提取金额）
    //   -?                : 可选的负号
    //   \d+               : 整数部分（至少一位数字）
    //   (?:\.\d+)?        : 小数部分（非捕获组），可选（例如 .50）
    // )                   : 第 1 捕获组结束
    // \s* : 匹配金额和描述之间的任意空白字符（0个或多个）
    // (.*)                : 第 2 捕获组（提取剩余所有字符作为描述和注释）
    std::regex re(R"(^(-?\d+(?:\.\d+)?)\s*(.*))");
    
    std::string full_description_part;

    if (std::regex_match(line, match, re) && match.size() == 3) {
        try {
            // match[1] 是金额字符串
            amount = std::stod(match[1].str());
            // match[2] 是描述部分（可能包含注释）
            full_description_part = match[2].str();
        } catch (const std::exception&) {
            amount = 0.0;
            full_description_part = line;
        }
    } else {
        // 如果正则匹配失败，金额归零，整行视为描述
        amount = 0.0;
        full_description_part = line;
    }

    // --- 处理注释分隔符 "//" ---
    size_t comment_pos = full_description_part.find("//");
    if (comment_pos != std::string::npos) {
        // 分隔符之后的内容为注释
        comment = full_description_part.substr(comment_pos + 2);
        // 分隔符之前的内容为描述
        description = full_description_part.substr(0, comment_pos);
    } else {
        description = full_description_part;
        comment = "";
    }
    
    // --- 清理首尾空白 ---
    // 移除描述末尾的空白（如 "肯德基 " -> "肯德基"）
    description.erase(description.find_last_not_of(" \t\n\r") + 1);
    // 移除注释开头的空白（如 " 午饭" -> "午饭"）
    comment.erase(0, comment.find_first_not_of(" \t\n\r"));
}