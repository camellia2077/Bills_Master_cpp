// bills_io/adapters/db/database_manager.hpp

#ifndef BILLS_IO_ADAPTERS_DB_DATABASE_MANAGER_H_
#define BILLS_IO_ADAPTERS_DB_DATABASE_MANAGER_H_

#include <sqlite3.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "billing/structures/common_data.hpp"

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
  void delete_bill_by_year_month(int year, int month);
  sqlite3_int64 insert_bill_record(const ParsedBill& bill_data);
  void insert_transactions_for_bill(
      sqlite3_int64 bill_id, const std::vector<Transaction>& transactions);

 private:
  sqlite3* m_db;  // SQLite 数据库连接句柄
};

#endif  // BILLS_IO_ADAPTERS_DB_DATABASE_MANAGER_H_

