// src/db_insert/parser/BillJsonParser.cpp

#include "BillJsonParser.hpp"
#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

ParsedBill BillJsonParser::parse(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开JSON文件: " + file_path);
    }

    json data;
    try {
        file >> data;
    } catch (json::parse_error& e) {
        throw std::runtime_error("解析JSON失败: " + std::string(e.what()));
    }

    ParsedBill bill_data;

    try {
        // 1. 解析顶层元数据
        std::string date_str = data.at("date").get<std::string>();
        bill_data.date = date_str;
        bill_data.remark = data.at("remark").get<std::string>();
        
        // --- 新增：从JSON读取总金额 ---
        bill_data.total_amount = data.at("total_amount").get<double>(); // <--- 读取 total_amount

        // 从日期字符串中提取年和月
        if (date_str.length() == 6) {
            bill_data.year = std::stoi(date_str.substr(0, 4));
            bill_data.month = std::stoi(date_str.substr(4, 2));
        } else {
            throw std::runtime_error("JSON中的日期格式无效，必须为 YYYYMM 格式。");
        }

        // 2. 遍历 categories 对象以解析交易
        const auto& categories = data.at("categories");
        for (const auto& parent_item : categories.items()) {
            const std::string& parent_category = parent_item.key();
            const auto& parent_data = parent_item.value();

            const auto& transactions_json = parent_data.at("transactions");

            for (const auto& item : transactions_json) {
                Transaction t;
                t.parent_category = parent_category;
                t.sub_category = item.at("sub_category").get<std::string>();
                t.description = item.at("description").get<std::string>();
                t.amount = item.at("amount").get<double>();
                t.source = item.value("source", "manually_add");
                t.comment = item.value("comment", "");
                
                // ===================================================================
                //  **修改：解析 transaction_type 字段**
                // ===================================================================
                t.transaction_type = item.value("transaction_type", "Expense"); // 默认为 "Expense"
                // ===================================================================

                bill_data.transactions.push_back(t);
            }
        }
    } catch (json::exception& e) {
        throw std::runtime_error("JSON 数据结构不符合预期: " + std::string(e.what()));
    }

    return bill_data;
}