#include "MonthlyQuery.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <sstream> // **MODIFIED**: Used to build the string report

MonthlyQuery::MonthlyQuery(sqlite3* db_connection) : m_db(db_connection) {}

// **MODIFIED**: The method now builds and returns a string based on integer year and month.
std::string MonthlyQuery::generate_report(int year, int month) {
    std::stringstream ss; // Use a stringstream to build the report

    // 步骤 1 & 2: 从数据库获取数据并在内存中聚合
    const char* sql = "SELECT parent_category, sub_category, amount, description, remark FROM transactions WHERE year = ? AND month = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备月度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_int(stmt, 1, year);
    sqlite3_bind_int(stmt, 2, month);

    std::map<std::string, ParentCategoryData> aggregated_data;
    std::string remark_text;
    double grand_total = 0.0;
    bool first_row = true;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string parent_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string sub_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        double amount = sqlite3_column_double(stmt, 2);
        const unsigned char* desc_raw = sqlite3_column_text(stmt, 3);

        Transaction t;
        t.parent_category = parent_cat;
        t.sub_category = sub_cat;
        t.amount = amount;
        t.description = desc_raw ? reinterpret_cast<const char*>(desc_raw) : "";

        aggregated_data[parent_cat].parent_total += amount;
        aggregated_data[parent_cat].sub_categories[sub_cat].sub_total += amount;
        aggregated_data[parent_cat].sub_categories[sub_cat].transactions.push_back(t);

        grand_total += amount;

        if (first_row) {
            const unsigned char* remark_raw = sqlite3_column_text(stmt, 4);
            remark_text = remark_raw ? reinterpret_cast<const char*>(remark_raw) : "";
            first_row = false;
        }
    }
    sqlite3_finalize(stmt);

    if (aggregated_data.empty()) {
        ss << "\n未找到 " << year << "年" << month << "月的任何数据。\n";
        return ss.str(); // Return the "not found" message
    }

    // 步骤 3: 排序
    for (auto& parent_pair : aggregated_data) {
        for (auto& sub_pair : parent_pair.second.sub_categories) {
            std::sort(sub_pair.second.transactions.begin(), sub_pair.second.transactions.end(),
                [](const Transaction& a, const Transaction& b) {
                    return a.amount > b.amount;
                });
        }
    }

    std::vector<std::pair<std::string, ParentCategoryData>> sorted_parents;
    for (const auto& pair : aggregated_data) {
        sorted_parents.push_back(pair);
    }
    std::sort(sorted_parents.begin(), sorted_parents.end(),
        [](const auto& a, const auto& b) {
            return a.second.parent_total > b.second.parent_total;
        });

    // --- 步骤 4: 按最终格式构建报告字符串 ---
    ss << std::fixed << std::setprecision(2);
    ss << "\n# DATE:" << year << std::setfill('0') << std::setw(2) << month << std::endl;
    ss << "# TOTAL:¥" << grand_total << std::endl;
    ss << "# REMARK:" << remark_text << std::endl;

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        ss << "\n# " << parent_name << std::endl;
        double parent_percentage = (grand_total > 0) ? (parent_data.parent_total / grand_total) * 100.0 : 0.0;
        ss << "总计：¥" << parent_data.parent_total << std::endl;
        ss << "占比：" << parent_percentage << "%" << std::endl;

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;

            ss << "\n## " << sub_name << std::endl;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << "小计：¥" << sub_data.sub_total << "（占比：" << sub_percentage << "%）" << std::endl;

            for (const auto& t : sub_data.transactions) {
                ss << "- ¥" << t.amount << " " << t.description << std::endl;
            }
        }
    }

    return ss.str(); // Return the completed report
}