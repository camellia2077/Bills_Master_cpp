// src/db_insert/insertor/DatabaseManager.cpp

#include "DatabaseManager.hpp"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string& db_path) : m_db(nullptr) {
    if (sqlite3_open(db_path.c_str(), &m_db) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db); // 确保在抛出异常前关闭句柄
        throw std::runtime_error("无法打开数据库: " + errmsg);
    }
    // 启用外键约束
    if (sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("无法启用外键支持: " + std::string(sqlite3_errmsg(m_db)));
    }
}

DatabaseManager::~DatabaseManager() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void DatabaseManager::initialize_database() {
    char* errmsg = nullptr;
    const char* create_bills_sql =
        "CREATE TABLE IF NOT EXISTS bills ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " bill_date TEXT NOT NULL UNIQUE,"
        " year INTEGER NOT NULL,"
        " month INTEGER NOT NULL,"
        " remark TEXT,"
        " total_amount REAL NOT NULL DEFAULT 0"
        ");";
    if (sqlite3_exec(m_db, create_bills_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::string error_str = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("无法创建 bills 表: " + error_str);
    }

    // ===================================================================
    //  **修改：在 transactions 表中添加 transaction_type 列**
    // ===================================================================
    const char* create_transactions_sql =
        "CREATE TABLE IF NOT EXISTS transactions ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " bill_id INTEGER NOT NULL,"
        " parent_category TEXT NOT NULL,"
        " sub_category TEXT NOT NULL,"
        " description TEXT,"
        " amount REAL NOT NULL,"
        " source TEXT NOT NULL DEFAULT 'manually_add',"
        " comment TEXT,"
        " transaction_type TEXT NOT NULL DEFAULT 'Expense'," // <-- 新增列
        " FOREIGN KEY (bill_id) REFERENCES bills(id) ON DELETE CASCADE"
        ");";
    // ===================================================================

    if (sqlite3_exec(m_db, create_transactions_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::string error_str = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("无法创建 transactions 表: " + error_str);
    }
}

void DatabaseManager::begin_transaction() {
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("无法开始事务: " + std::string(sqlite3_errmsg(m_db)));
    }
}

void DatabaseManager::commit_transaction() {
    if (sqlite3_exec(m_db, "COMMIT;", 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("提交事务失败: " + std::string(sqlite3_errmsg(m_db)));
    }
}

void DatabaseManager::rollback_transaction() {
    sqlite3_exec(m_db, "ROLLBACK;", 0, 0, 0);
}

void DatabaseManager::delete_bill_by_date(const std::string& date) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM bills WHERE bill_date = ?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 DELETE 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("执行 DELETE 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_finalize(stmt);
}

sqlite3_int64 DatabaseManager::insert_bill_record(const ParsedBill& bill_data) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO bills (bill_date, year, month, remark, total_amount) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 INSERT bill 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_text(stmt, 1, bill_data.date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, bill_data.year);
    sqlite3_bind_int(stmt, 3, bill_data.month);
    sqlite3_bind_text(stmt, 4, bill_data.remark.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 5, bill_data.total_amount);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("插入 bill 数据失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(m_db);
}

void DatabaseManager::insert_transactions_for_bill(sqlite3_int64 bill_id, const std::vector<Transaction>& transactions) {
    sqlite3_stmt* stmt = nullptr;
    
    // ===================================================================
    //  **修改：更新 INSERT 语句以包含 transaction_type**
    // ===================================================================
    const char* sql =
        "INSERT INTO transactions (bill_id, parent_category, sub_category, description, amount, source, comment, transaction_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    // ===================================================================

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 INSERT transaction 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    for (const auto& transaction : transactions) {
        sqlite3_bind_int64(stmt, 1, bill_id);
        sqlite3_bind_text(stmt, 2, transaction.parent_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, transaction.sub_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, transaction.description.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 5, transaction.amount);
        sqlite3_bind_text(stmt, 6, transaction.source.c_str(), -1, SQLITE_STATIC);
        if (transaction.comment.empty()) {
            sqlite3_bind_null(stmt, 7);
        } else {
            sqlite3_bind_text(stmt, 7, transaction.comment.c_str(), -1, SQLITE_STATIC);
        }
        
        // ===================================================================
        //  **修改：绑定 transaction_type 的值**
        // ===================================================================
        sqlite3_bind_text(stmt, 8, transaction.transaction_type.c_str(), -1, SQLITE_STATIC);
        // ===================================================================

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("插入 transaction 数据失败: " + std::string(sqlite3_errmsg(m_db)));
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
}