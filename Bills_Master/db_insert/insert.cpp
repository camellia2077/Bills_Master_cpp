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

// --- LOGIC MODIFIED ---
// The entire operation is now wrapped in a single transaction.
// It first deletes all records for the given month before inserting the new ones.
void BillInserter::insert_bill(const ParsedBill& bill_data) {
    if (bill_data.date.empty()) {
        throw std::runtime_error("无法插入日期为空的账单。");
    }

    // 开始事务，这将保证删除和插入操作的原子性
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("无法开始事务: " + std::string(sqlite3_errmsg(m_db)));
    }

    try {
        // 步骤 1: 删除该月份的所有现有记录
        sqlite3_stmt* delete_stmt = nullptr;
        const char* delete_sql = "DELETE FROM transactions WHERE bill_date = ?;";
        if (sqlite3_prepare_v2(m_db, delete_sql, -1, &delete_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("准备 DELETE 语句失败: " + std::string(sqlite3_errmsg(m_db)));
        }
        sqlite3_bind_text(delete_stmt, 1, bill_data.date.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
            sqlite3_finalize(delete_stmt);
            throw std::runtime_error("执行 DELETE 语句失败: " + std::string(sqlite3_errmsg(m_db)));
        }
        sqlite3_finalize(delete_stmt);


        // 步骤 2: 插入所有新记录
        sqlite3_stmt* insert_stmt = nullptr;
        const char* insert_sql =
            "INSERT INTO transactions (bill_date, parent_category, sub_category, amount, description, remark) "
            "VALUES (?, ?, ?, ?, ?, ?);";
        
        if (sqlite3_prepare_v2(m_db, insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("准备 INSERT 语句失败: " + std::string(sqlite3_errmsg(m_db)));
        }

        for (const auto& transaction : bill_data.transactions) {
            sqlite3_bind_text(insert_stmt, 1, bill_data.date.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insert_stmt, 2, transaction.parent_category.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insert_stmt, 3, transaction.sub_category.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(insert_stmt, 4, transaction.amount);
            sqlite3_bind_text(insert_stmt, 5, transaction.description.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insert_stmt, 6, bill_data.remark.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
                sqlite3_finalize(insert_stmt);
                throw std::runtime_error("插入数据失败: " + std::string(sqlite3_errmsg(m_db)));
            }
            sqlite3_reset(insert_stmt);
        }
        sqlite3_finalize(insert_stmt);

        // 步骤 3: 提交事务
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