// bills_io/adapters/db/database_manager.cpp

#include "database_manager.hpp"

#include <iostream>

namespace {
constexpr int kDeleteBillYearIndex = 1;
constexpr int kDeleteBillMonthIndex = 2;

constexpr int kInsertBillDateIndex = 1;
constexpr int kInsertBillYearIndex = 2;
constexpr int kInsertBillMonthIndex = 3;
constexpr int kInsertBillRemarkIndex = 4;
constexpr int kInsertBillTotalIncomeIndex = 5;
constexpr int kInsertBillTotalExpenseIndex = 6;
constexpr int kInsertBillBalanceIndex = 7;

constexpr int kInsertTransactionBillIdIndex = 1;
constexpr int kInsertTransactionParentCategoryIndex = 2;
constexpr int kInsertTransactionSubCategoryIndex = 3;
constexpr int kInsertTransactionDescriptionIndex = 4;
constexpr int kInsertTransactionAmountIndex = 5;
constexpr int kInsertTransactionSourceIndex = 6;
constexpr int kInsertTransactionCommentIndex = 7;
constexpr int kInsertTransactionTypeIndex = 8;
}  // namespace

DatabaseManager::DatabaseManager(const std::string& db_path) : m_db(nullptr) {
  if (sqlite3_open(db_path.c_str(), &m_db) != SQLITE_OK) {
    std::string errmsg = sqlite3_errmsg(m_db);
    sqlite3_close(m_db);
    throw std::runtime_error("无法打开数据库: " + errmsg);
  }
  if (sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr,
                   nullptr) != SQLITE_OK) {
    throw std::runtime_error("无法启用外键支持: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
}

DatabaseManager::~DatabaseManager() {
  if (m_db != nullptr) {
    sqlite3_close(m_db);
  }
}

void DatabaseManager::initialize_database() {
  char* errmsg = nullptr;
  // --- 【核心修改 1】 ---
  // 更新 bills 表的结构，用三个新字段替换 total_amount
  const char* create_bills_sql =
      "CREATE TABLE IF NOT EXISTS bills ("
      " id INTEGER PRIMARY KEY AUTOINCREMENT,"
      " bill_date TEXT NOT NULL UNIQUE,"
      " year INTEGER NOT NULL CHECK(year BETWEEN 1900 AND 9999),"
      " month INTEGER NOT NULL CHECK(month BETWEEN 1 AND 12),"
      " remark TEXT,"
      " total_income REAL NOT NULL DEFAULT 0,"
      " total_expense REAL NOT NULL DEFAULT 0,"
      " balance REAL NOT NULL DEFAULT 0,"
      " CHECK(bill_date = printf('%04d-%02d', year, month))"
      ");";
  // --- 修改结束 ---
  if (sqlite3_exec(m_db, create_bills_sql, nullptr, nullptr, &errmsg) !=
      SQLITE_OK) {
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
      " description TEXT,"
      " amount REAL NOT NULL,"
      " source TEXT NOT NULL DEFAULT 'manually_add',"
      " comment TEXT,"
      " transaction_type TEXT NOT NULL DEFAULT 'Expense',"
      " FOREIGN KEY (bill_id) REFERENCES bills(id) ON DELETE CASCADE"
      ");";

  if (sqlite3_exec(m_db, create_transactions_sql, nullptr, nullptr, &errmsg) !=
      SQLITE_OK) {
    std::string error_str = errmsg;
    sqlite3_free(errmsg);
    throw std::runtime_error("无法创建 transactions 表: " + error_str);
  }
}

void DatabaseManager::begin_transaction() {
  if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("无法开始事务: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
}

void DatabaseManager::commit_transaction() {
  if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
    throw std::runtime_error("提交事务失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
}

void DatabaseManager::rollback_transaction() {
  sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
}

void DatabaseManager::delete_bill_by_year_month(int year, int month) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql = "DELETE FROM bills WHERE year = ? AND month = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("准备 DELETE 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
  sqlite3_bind_int(stmt, kDeleteBillYearIndex, year);
  sqlite3_bind_int(stmt, kDeleteBillMonthIndex, month);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error("执行 DELETE 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
  sqlite3_finalize(stmt);
}

auto DatabaseManager::insert_bill_record(const ParsedBill& bill_data)
    -> sqlite3_int64 {
  sqlite3_stmt* stmt = nullptr;
  // --- 【核心修改 2】 ---
  // 更新 INSERT 语句以匹配新的表结构
  const char* sql =
      "INSERT INTO bills (bill_date, year, month, remark, total_income, "
      "total_expense, balance) VALUES (?, ?, ?, ?, ?, ?, ?);";
  // --- 修改结束 ---
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("准备 INSERT bill 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }

  sqlite3_bind_text(stmt, kInsertBillDateIndex, bill_data.date.c_str(), -1,
                    SQLITE_STATIC);
  sqlite3_bind_int(stmt, kInsertBillYearIndex, bill_data.year);
  sqlite3_bind_int(stmt, kInsertBillMonthIndex, bill_data.month);
  sqlite3_bind_text(stmt, kInsertBillRemarkIndex, bill_data.remark.c_str(), -1,
                    SQLITE_STATIC);
  // --- 【核心修改 3】 ---
  // 绑定新的总计字段到SQL语句
  sqlite3_bind_double(stmt, kInsertBillTotalIncomeIndex,
                      bill_data.total_income);
  sqlite3_bind_double(stmt, kInsertBillTotalExpenseIndex,
                      bill_data.total_expense);
  sqlite3_bind_double(stmt, kInsertBillBalanceIndex, bill_data.balance);
  // --- 修改结束 ---

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error("插入 bill 数据失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
  sqlite3_finalize(stmt);
  return sqlite3_last_insert_rowid(m_db);
}

void DatabaseManager::insert_transactions_for_bill(
    sqlite3_int64 bill_id, const std::vector<Transaction>& transactions) {
  sqlite3_stmt* stmt = nullptr;

  const char* sql =
      "INSERT INTO transactions (bill_id, parent_category, sub_category, "
      "description, amount, source, comment, transaction_type) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("准备 INSERT transaction 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }

  for (const auto& transaction : transactions) {
    sqlite3_bind_int64(stmt, kInsertTransactionBillIdIndex, bill_id);
    sqlite3_bind_text(stmt, kInsertTransactionParentCategoryIndex,
                      transaction.parent_category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, kInsertTransactionSubCategoryIndex,
                      transaction.sub_category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, kInsertTransactionDescriptionIndex,
                      transaction.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, kInsertTransactionAmountIndex,
                        transaction.amount);
    sqlite3_bind_text(stmt, kInsertTransactionSourceIndex,
                      transaction.source.c_str(), -1, SQLITE_STATIC);
    if (transaction.comment.empty()) {
      sqlite3_bind_null(stmt, kInsertTransactionCommentIndex);
    } else {
      sqlite3_bind_text(stmt, kInsertTransactionCommentIndex,
                        transaction.comment.c_str(), -1, SQLITE_STATIC);
    }

    sqlite3_bind_text(stmt, kInsertTransactionTypeIndex,
                      transaction.transaction_type.c_str(), -1,
                      SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      throw std::runtime_error("插入 transaction 数据失败: " +
                               std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_reset(stmt);
  }
  sqlite3_finalize(stmt);
}
