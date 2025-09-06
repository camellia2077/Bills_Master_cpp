#include "BillInserter.hpp"
#include <iostream>

BillInserter::BillInserter(const std::string& db_path) : m_db(nullptr) {
    if (sqlite3_open(db_path.c_str(), &m_db) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw std::runtime_error("无法打开数据库: " + errmsg);
    }
    if (sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", 0, 0, 0) != SQLITE_OK) {
         throw std::runtime_error("无法启用外键支持: " + std::string(sqlite3_errmsg(m_db)));
    }
    initialize_database();
}

BillInserter::~BillInserter() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void BillInserter::initialize_database() {
    char* errmsg = nullptr;
    const char* create_bills_sql =
        "CREATE TABLE IF NOT EXISTS bills ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " bill_date TEXT NOT NULL UNIQUE,"
        " year INTEGER NOT NULL,"
        " month INTEGER NOT NULL,"
        " remark TEXT"
        ");";
    if (sqlite3_exec(m_db, create_bills_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::string error_str = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("无法创建 bills 表: " + error_str);
    }
    
    // --- 修改：在 transactions 表中添加 source 字段 ---
    const char* create_transactions_sql =
        "CREATE TABLE IF NOT EXISTS transactions ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " bill_id INTEGER NOT NULL,"
        " parent_category TEXT NOT NULL,"
        " sub_category TEXT NOT NULL,"
        " amount REAL NOT NULL,"
        " description TEXT,"
        " source TEXT NOT NULL DEFAULT 'manually_add'," // 新增 source 字段
        " FOREIGN KEY (bill_id) REFERENCES bills(id) ON DELETE CASCADE"
        ");";
    if (sqlite3_exec(m_db, create_transactions_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::string error_str = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("无法创建 transactions 表: " + error_str);
    }
}

void BillInserter::insert_bill(const ParsedBill& bill_data) {
    if (bill_data.date.empty()) {
        throw std::runtime_error("无法插入日期为空的账单。");
    }
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("无法开始事务: " + std::string(sqlite3_errmsg(m_db)));
    }
    try {
        delete_bill_by_date(bill_data.date);
        sqlite3_int64 bill_id = insert_bill_record(bill_data);
        insert_transactions_for_bill(bill_id, bill_data.transactions);
        if (sqlite3_exec(m_db, "COMMIT;", 0, 0, 0) != SQLITE_OK) {
            throw std::runtime_error("提交事务失败: " + std::string(sqlite3_errmsg(m_db)));
        }
    } catch (const std::exception& e) {
        sqlite3_exec(m_db, "ROLLBACK;", 0, 0, 0);
        throw;
    }
}

void BillInserter::delete_bill_by_date(const std::string& date) {
    sqlite3_stmt* delete_stmt = nullptr;
    const char* delete_sql = "DELETE FROM bills WHERE bill_date = ?;";
    if (sqlite3_prepare_v2(m_db, delete_sql, -1, &delete_stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 DELETE 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_bind_text(delete_stmt, 1, date.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
        sqlite3_finalize(delete_stmt);
        throw std::runtime_error("执行 DELETE 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_finalize(delete_stmt);
}

sqlite3_int64 BillInserter::insert_bill_record(const ParsedBill& bill_data) {
    sqlite3_stmt* insert_stmt = nullptr;
    const char* insert_sql = "INSERT INTO bills (bill_date, year, month, remark) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db, insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 INSERT bill 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_bind_text(insert_stmt, 1, bill_data.date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(insert_stmt, 2, bill_data.year);
    sqlite3_bind_int(insert_stmt, 3, bill_data.month);
    sqlite3_bind_text(insert_stmt, 4, bill_data.remark.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
        sqlite3_finalize(insert_stmt);
        throw std::runtime_error("插入 bill 数据失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_finalize(insert_stmt);
    return sqlite3_last_insert_rowid(m_db);
}

void BillInserter::insert_transactions_for_bill(sqlite3_int64 bill_id, const std::vector<Transaction>& transactions) {
    sqlite3_stmt* insert_stmt = nullptr;
    // --- 修改：更新 INSERT 语句以包含 source ---
    const char* insert_sql =
        "INSERT INTO transactions (bill_id, parent_category, sub_category, amount, description, source) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db, insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 INSERT transaction 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    for (const auto& transaction : transactions) {
        sqlite3_bind_int64(insert_stmt, 1, bill_id);
        sqlite3_bind_text(insert_stmt, 2, transaction.parent_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 3, transaction.sub_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insert_stmt, 4, transaction.amount);
        sqlite3_bind_text(insert_stmt, 5, transaction.description.c_str(), -1, SQLITE_STATIC);
        // --- 新增：绑定 source 的值 ---
        sqlite3_bind_text(insert_stmt, 6, transaction.source.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
            sqlite3_finalize(insert_stmt);
            throw std::runtime_error("插入 transaction 数据失败: " + std::string(sqlite3_errmsg(m_db)));
        }
        sqlite3_reset(insert_stmt);
    }
    sqlite3_finalize(insert_stmt);
}