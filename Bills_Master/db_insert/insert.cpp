#include "insert.h"
#include <iostream>

BillInserter::BillInserter(const std::string& db_path) : m_db(nullptr) {
    // 打开数据库连接
    if (sqlite3_open(db_path.c_str(), &m_db) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw std::runtime_error("无法打开数据库: " + errmsg);
    }
    // 初始化数据库，创建表
    initialize_database();
}

BillInserter::~BillInserter() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void BillInserter::initialize_database() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  bill_date TEXT NOT NULL,"
        "  parent_category TEXT NOT NULL,"
        "  sub_category TEXT NOT NULL,"
        "  amount REAL NOT NULL,"
        "  description TEXT,"
        "  remark TEXT"
        ");";

    char* errmsg = nullptr;
    if (sqlite3_exec(m_db, sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::string error_str = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("无法创建表: " + error_str);
    }
}

void BillInserter::insert_bill(const ParsedBill& bill_data) {
    // 使用预处理语句以防止 SQL 注入并提高性能
    const char* sql =
        "INSERT INTO transactions (bill_date, parent_category, sub_category, amount, description, remark) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("无法准备 SQL 语句: " + std::string(sqlite3_errmsg(m_db)));
    }

    // 使用事务来批量插入，极大提高效率
    sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0);

    for (const auto& transaction : bill_data.transactions) {
        // 绑定参数
        sqlite3_bind_text(stmt, 1, bill_data.date.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, transaction.parent_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, transaction.sub_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, transaction.amount);
        sqlite3_bind_text(stmt, 5, transaction.description.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, bill_data.remark.c_str(), -1, SQLITE_STATIC);

        // 执行语句
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "插入失败: " << sqlite3_errmsg(m_db) << std::endl;
        }

        // 重置语句以便下一次循环使用
        sqlite3_reset(stmt);
    }
    
    // 提交事务
    sqlite3_exec(m_db, "COMMIT;", 0, 0, 0);

    // 释放语句句柄
    sqlite3_finalize(stmt);
}