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
        std::string date_str = data.at("date").get<std::string>();
        bill_data.date = date_str;
        bill_data.remark = data.at("remark").get<std::string>();

        if (date_str.length() == 6) {
            bill_data.year = std::stoi(date_str.substr(0, 4));
            bill_data.month = std::stoi(date_str.substr(4, 2));
        } else {
            throw std::runtime_error("JSON中的日期格式无效，必须为 YYYYMM 格式。");
        }

        const auto& categories = data.at("categories");
        for (auto it = categories.begin(); it != categories.end(); ++it) {
            const std::string& parent_category = it.key();
            const auto& sub_categories = it.value();

            for (auto sub_it = sub_categories.begin(); sub_it != sub_categories.end(); ++sub_it) {
                const std::string& sub_category = sub_it.key();
                const auto& transactions_json = sub_it.value();

                for (const auto& item : transactions_json) {
                    Transaction t;
                    t.parent_category = parent_category;
                    t.sub_category = sub_category;
                    t.description = item.at("description").get<std::string>();
                    t.amount = item.at("amount").get<double>();
                    t.source = item.value("source", "manually_add");
                    // --- 新增：解析 comment 字段 ---
                    // 如果 "comment" 键不存在或为 null，则默认为空字符串
                    t.comment = item.value("comment", ""); 
                    bill_data.transactions.push_back(t);
                }
            }
        }
    } catch (json::exception& e) {
        throw std::runtime_error("JSON 数据结构不符合预期: " + std::string(e.what()));
    }

    return bill_data;
}