// src/db_insert/insertor/DatabaseManager.hpp

#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "common_structures/CommonData.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdexcept>

/**
 * @class DatabaseManager
 * @brief 负责所有底层的数据库交互，包括连接、Schema管理和CRUD操作。
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& db_path);
    ~DatabaseManager();

    // 禁止拷贝和赋值，因为数据库连接是独占资源
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // --- Schema Management ---
    void initialize_database();

    // --- Transaction Management ---
    void begin_transaction();
    void commit_transaction();
    void rollback_transaction();

    // --- Data Manipulation (CRUD) ---
    void delete_bill_by_date(const std::string& date);
    sqlite3_int64 insert_bill_record(const ParsedBill& bill_data);
    void insert_transactions_for_bill(sqlite3_int64 bill_id, const std::vector<Transaction>& transactions);

private:
    sqlite3* m_db; // SQLite 数据库连接句柄
};

#endif // DATABASE_MANAGER_H