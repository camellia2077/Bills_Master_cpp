
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
    // ... 此方法的实现保持不变 ...
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
    const char* create_transactions_sql =
        "CREATE TABLE IF NOT EXISTS transactions ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " bill_id INTEGER NOT NULL,"
        " parent_category TEXT NOT NULL,"
        " sub_category TEXT NOT NULL,"
        " amount REAL NOT NULL,"
        " description TEXT,"
        " FOREIGN KEY (bill_id) REFERENCES bills(id) ON DELETE CASCADE"
        ");";
    if (sqlite3_exec(m_db, create_transactions_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::string error_str = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("无法创建 transactions 表: " + error_str);
    }
}

// --- 方法已被重构 ---
// 公共方法现在是一个高层次的协调者，负责事务和调用辅助方法。
void BillInserter::insert_bill(const ParsedBill& bill_data) {
    if (bill_data.date.empty()) {
        throw std::runtime_error("无法插入日期为空的账单。");
    }

    // 开始事务，保证所有操作的原子性
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("无法开始事务: " + std::string(sqlite3_errmsg(m_db)));
    }

    try {
        // 步骤 1: 删除旧记录 (单一职责)
        delete_bill_by_date(bill_data.date);

        // 步骤 2: 插入新的父记录并获取ID (单一职责)
        sqlite3_int64 bill_id = insert_bill_record(bill_data);

        // 步骤 3: 插入所有子记录 (单一职责)
        insert_transactions_for_bill(bill_id, bill_data.transactions);

        // 步骤 4: 提交事务
        if (sqlite3_exec(m_db, "COMMIT;", 0, 0, 0) != SQLITE_OK) {
            throw std::runtime_error("提交事务失败: " + std::string(sqlite3_errmsg(m_db)));
        }

    } catch (const std::exception& e) {
        // 如果在 try 块中发生任何错误，回滚事务
        sqlite3_exec(m_db, "ROLLBACK;", 0, 0, 0);
        // 将异常重新抛出，以便上层调用者知道操作失败
        throw;
    }
}

// --- 新的私有辅助方法 ---

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
    const char* insert_sql =
        "INSERT INTO transactions (bill_id, parent_category, sub_category, amount, description) "
        "VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db, insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 INSERT transaction 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    for (const auto& transaction : transactions) {
        sqlite3_bind_int64(insert_stmt, 1, bill_id);
        sqlite3_bind_text(insert_stmt, 2, transaction.parent_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 3, transaction.sub_category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insert_stmt, 4, transaction.amount);
        sqlite3_bind_text(insert_stmt, 5, transaction.description.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
            sqlite3_finalize(insert_stmt);
            throw std::runtime_error("插入 transaction 数据失败: " + std::string(sqlite3_errmsg(m_db)));
        }
        sqlite3_reset(insert_stmt);
    }
    sqlite3_finalize(insert_stmt);
}