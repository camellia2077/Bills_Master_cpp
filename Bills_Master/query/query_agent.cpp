#include "query_agent.h"
#include <iostream>
#include <iomanip> // For std::setw, std::left
#include <vector>  // To store results before printing

QueryAgent::QueryAgent(const std::string& db_path) : m_db(nullptr) {
    // 以只读模式打开数据库，更安全
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db); // sqlite3_close can be called on a nullptr safely
        throw std::runtime_error("无法以只读模式打开数据库: " + errmsg);
    }
}

QueryAgent::~QueryAgent() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void QueryAgent::display_yearly_summary(const std::string& year) {
    std::cout << "\n--- " << year << " 年各月份支出总览 ---\n";

    // SQL 查询：按月份分组并计算总金额，仅筛选指定年份
    const char* sql = "SELECT bill_date, SUM(amount) FROM transactions WHERE bill_date LIKE ? GROUP BY bill_date ORDER BY bill_date;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备年度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    // 绑定年份参数，例如 "2025%"
    std::string year_pattern = year + "%";
    sqlite3_bind_text(stmt, 1, year_pattern.c_str(), -1, SQLITE_STATIC);

    bool found_data = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found_data = true;
        const unsigned char* month_raw = sqlite3_column_text(stmt, 0);
        double total_amount = sqlite3_column_double(stmt, 1);
        
        std::cout << "月份: " << month_raw 
                  << ", 总金额: " << std::fixed << std::setprecision(2) << total_amount << std::endl;
    }
    
    if (!found_data) {
        std::cout << "未找到 " << year << " 年的任何数据。\n";
    }

    sqlite3_finalize(stmt);
    std::cout << "--- 查询结束 ---\n";
}


// --- 函数已重构 ---
void QueryAgent::display_monthly_details(const std::string& month) {
    // 步骤 1: 修改 SQL，获取所有需要的数据并排序
    const char* sql = "SELECT parent_category, sub_category, amount, description, remark FROM transactions WHERE bill_date = ? ORDER BY parent_category, sub_category;";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备月度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_STATIC);

    // 步骤 2: 将所有数据先读入内存
    std::vector<Transaction> transactions;
    std::string remark_text;
    double total_amount = 0.0;
    bool first_row = true;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Transaction t;
        t.parent_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t.sub_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.amount = sqlite3_column_double(stmt, 2);
        const unsigned char* desc_raw = sqlite3_column_text(stmt, 3);
        t.description = desc_raw ? reinterpret_cast<const char*>(desc_raw) : "";
        
        transactions.push_back(t);
        total_amount += t.amount;

        // 从第一行获取备注信息
        if (first_row) {
            const unsigned char* remark_raw = sqlite3_column_text(stmt, 4);
            remark_text = remark_raw ? reinterpret_cast<const char*>(remark_raw) : "";
            first_row = false;
        }
    }
    sqlite3_finalize(stmt);

    // 步骤 3: 检查是否有数据
    if (transactions.empty()) {
        std::cout << "\n未找到 " << month << " 月的任何数据。\n";
        return;
    }

    // 步骤 4: 按 Markdown 格式打印
    std::cout << "\n# DATE:" << month << std::endl;
    std::cout << "# TOTAL:" << std::fixed << std::setprecision(2) << total_amount << std::endl;
    std::cout << "# REMARK:" << remark_text << std::endl;

    std::string current_parent;
    std::string current_sub;

    for (const auto& t : transactions) {
        // 当父类别变化时，打印父类别标题
        if (t.parent_category != current_parent) {
            current_parent = t.parent_category;
            std::cout << "\n# " << current_parent << std::endl;
            current_sub = ""; // 重置子类别，以确保子类别标题能被打印
        }
        // 当子类别变化时，打印子类别标题
        if (t.sub_category != current_sub) {
            current_sub = t.sub_category;
            std::cout << "## " << current_sub << std::endl;
        }
        // 打印交易详情
        std::cout << "- " << std::fixed << std::setprecision(2) << t.amount << " " << t.description << std::endl;
    }
}