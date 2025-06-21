#include "Database_Inserter.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>

// 构造函数和析构函数保持不变
DatabaseInserter::DatabaseInserter(const std::string& db_path) : db_path_(db_path) {
    if (sqlite3_open(db_path.c_str(), &db_)) {
        std::string errMsg = "Can't open database: ";
        errMsg += sqlite3_errmsg(db_);
        throw std::runtime_error(errMsg);
    }
}

DatabaseInserter::~DatabaseInserter() {
    if (db_) {
        sqlite3_close(db_);
    }
}

// 辅助函数和事务管理保持不变
void DatabaseInserter::execute_sql(const std::string& sql) {
    char* zErrMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), 0, 0, &zErrMsg) != SQLITE_OK) {
        std::string errMsg = "SQL error: " + std::string(zErrMsg);
        sqlite3_free(zErrMsg);
        throw std::runtime_error(errMsg);
    }
}

void DatabaseInserter::begin_transaction() {
    execute_sql("BEGIN TRANSACTION;");
}

void DatabaseInserter::commit_transaction() {
    execute_sql("COMMIT;");
}

void DatabaseInserter::rollback_transaction() {
    char* zErrMsg = nullptr;
    if (sqlite3_exec(db_, "ROLLBACK;", 0, 0, &zErrMsg) != SQLITE_OK) {
        std::cerr << "CRITICAL: Failed to rollback transaction: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
}

// create_database 保持不变
void DatabaseInserter::create_database() {
    const std::string schema =
        "CREATE TABLE IF NOT EXISTS YearMonth ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  value TEXT NOT NULL UNIQUE,"
        "  remark TEXT"
        ");"

        "CREATE TABLE IF NOT EXISTS Parent ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  year_month_id INTEGER NOT NULL,"
        "  order_ INTEGER NOT NULL,"
        "  title TEXT NOT NULL,"
        "  FOREIGN KEY (year_month_id) REFERENCES YearMonth(id),"
        "  UNIQUE(year_month_id, title)"
        ");"

        "CREATE TABLE IF NOT EXISTS Child ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  parent_id INTEGER NOT NULL,"
        "  order_ INTEGER NOT NULL,"
        "  title TEXT NOT NULL,"
        "  FOREIGN KEY (parent_id) REFERENCES Parent(id),"
        "  UNIQUE(parent_id, title)"
        ");"

        "CREATE TABLE IF NOT EXISTS Item ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  child_id INTEGER NOT NULL,"
        "  order_ INTEGER NOT NULL,"
        "  amount REAL NOT NULL,"
        "  description TEXT,"
        "  FOREIGN KEY (child_id) REFERENCES Child(id)"
        ");"

        "CREATE INDEX IF NOT EXISTS idx_parent_year_month_id ON Parent(year_month_id);"
        "CREATE INDEX IF NOT EXISTS idx_child_parent_id ON Child(parent_id);"
        "CREATE INDEX IF NOT EXISTS idx_item_child_id ON Item(child_id);";

    try {
        begin_transaction();
        execute_sql(schema);
        commit_transaction();
        std::cout << "Database schema created or verified successfully." << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "Schema creation failed, rolling back." << std::endl;
        rollback_transaction();
        throw;
    }
}

// --- 高性能优化的 insert_data_stream ---
void DatabaseInserter::insert_data_stream(const std::vector<ParsedRecord>& records) {
    // --- 1. 一次性准备所有 SQL 语句 ---
    sqlite3_stmt *upsert_ym_stmt = nullptr;
    sqlite3_stmt *select_ym_id_stmt = nullptr;
    sqlite3_stmt *update_remark_stmt = nullptr;
    sqlite3_stmt *upsert_parent_stmt = nullptr;
    sqlite3_stmt *select_parent_id_stmt = nullptr;
    sqlite3_stmt *upsert_child_stmt = nullptr;
    sqlite3_stmt *select_child_id_stmt = nullptr;
    sqlite3_stmt *insert_item_stmt = nullptr;

    const char* upsert_year_month_sql = "INSERT INTO YearMonth(value) VALUES (?) ON CONFLICT(value) DO NOTHING;";
    const char* select_year_month_id_sql = "SELECT id FROM YearMonth WHERE value = ?;";
    const char* update_remark_sql = "UPDATE YearMonth SET remark = ? WHERE id = ?;";
    const char* upsert_parent_sql = "INSERT INTO Parent(year_month_id, order_, title) VALUES (?, ?, ?) ON CONFLICT(year_month_id, title) DO UPDATE SET order_=excluded.order_;";
    const char* select_parent_id_sql = "SELECT id FROM Parent WHERE year_month_id = ? AND title = ?;";
    const char* upsert_child_sql = "INSERT INTO Child(parent_id, order_, title) VALUES (?, ?, ?) ON CONFLICT(parent_id, title) DO UPDATE SET order_=excluded.order_;";
    const char* select_child_id_sql = "SELECT id FROM Child WHERE parent_id = ? AND title = ?;";
    const char* insert_item_sql = "INSERT INTO Item(child_id, order_, amount, description) VALUES (?, ?, ?, ?);";

    // 辅助宏，用于检查 prepare 的结果
    #define PREPARE_STMT(db, sql, stmt_ptr) \
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_ptr, nullptr) != SQLITE_OK) { \
            throw std::runtime_error("Failed to prepare statement (" + std::string(sql) + "): " + sqlite3_errmsg(db)); \
        }

    try {
        // 在循环外编译所有语句
        PREPARE_STMT(db_, upsert_year_month_sql, upsert_ym_stmt);
        PREPARE_STMT(db_, select_year_month_id_sql, select_ym_id_stmt);
        PREPARE_STMT(db_, update_remark_sql, update_remark_stmt);
        PREPARE_STMT(db_, upsert_parent_sql, upsert_parent_stmt);
        PREPARE_STMT(db_, select_parent_id_sql, select_parent_id_stmt);
        PREPARE_STMT(db_, upsert_child_sql, upsert_child_stmt);
        PREPARE_STMT(db_, select_child_id_sql, select_child_id_stmt);
        PREPARE_STMT(db_, insert_item_sql, insert_item_stmt);

        long long current_year_month_id = -1;
        long long current_parent_id = -1;
        long long current_child_id = -1;
        const size_t BATCH_SIZE = 100;
        std::vector<ParsedRecord> item_batch;

        // --- 2. 优化后的批处理函数 ---
        auto flush_item_batch = [&](long long child_id) {
            if (item_batch.empty()) return;
            for (const auto& item_rec : item_batch) {
                sqlite3_bind_int64(insert_item_stmt, 1, child_id);
                sqlite3_bind_int(insert_item_stmt, 2, item_rec.order);
                sqlite3_bind_double(insert_item_stmt, 3, item_rec.amount);
                sqlite3_bind_text(insert_item_stmt, 4, item_rec.description.c_str(), -1, SQLITE_TRANSIENT);

                if (sqlite3_step(insert_item_stmt) != SQLITE_DONE) {
                    throw std::runtime_error("Failed to execute item insert statement: " + std::string(sqlite3_errmsg(db_)));
                }
                sqlite3_reset(insert_item_stmt);
                sqlite3_clear_bindings(insert_item_stmt); // 总是好习惯
            }
            item_batch.clear();
        };

        // --- 3. 使用预备语句的主处理循环 ---
        for (const auto& rec : records) {
            if (rec.type == "date") {
                flush_item_batch(current_child_id);
                current_parent_id = -1; current_child_id = -1;

                sqlite3_bind_text(upsert_ym_stmt, 1, rec.content.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(upsert_ym_stmt);
                sqlite3_reset(upsert_ym_stmt);

                sqlite3_bind_text(select_ym_id_stmt, 1, rec.content.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(select_ym_id_stmt) == SQLITE_ROW) {
                    current_year_month_id = sqlite3_column_int64(select_ym_id_stmt, 0);
                }
                sqlite3_reset(select_ym_id_stmt);

            } else if (rec.type == "remark") {
                if (current_year_month_id == -1) throw std::runtime_error("Hierarchical error: REMARK without DATE at line " + std::to_string(rec.lineNumber));
                sqlite3_bind_text(update_remark_stmt, 1, rec.content.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(update_remark_stmt, 2, current_year_month_id);
                sqlite3_step(update_remark_stmt);
                sqlite3_reset(update_remark_stmt);

            } else if (rec.type == "parent") {
                flush_item_batch(current_child_id);
                current_child_id = -1;
                if (current_year_month_id == -1) throw std::runtime_error("Hierarchical error: PARENT without DATE at line " + std::to_string(rec.lineNumber));

                sqlite3_bind_int64(upsert_parent_stmt, 1, current_year_month_id);
                sqlite3_bind_int(upsert_parent_stmt, 2, rec.order);
                sqlite3_bind_text(upsert_parent_stmt, 3, rec.content.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(upsert_parent_stmt);
                sqlite3_reset(upsert_parent_stmt);

                sqlite3_bind_int64(select_parent_id_stmt, 1, current_year_month_id);
                sqlite3_bind_text(select_parent_id_stmt, 2, rec.content.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(select_parent_id_stmt) == SQLITE_ROW) {
                    current_parent_id = sqlite3_column_int64(select_parent_id_stmt, 0);
                }
                sqlite3_reset(select_parent_id_stmt);

            } else if (rec.type == "child") {
                flush_item_batch(current_child_id);
                if (current_parent_id == -1) throw std::runtime_error("Hierarchical error: CHILD without PARENT at line " + std::to_string(rec.lineNumber));

                sqlite3_bind_int64(upsert_child_stmt, 1, current_parent_id);
                sqlite3_bind_int(upsert_child_stmt, 2, rec.order);
                sqlite3_bind_text(upsert_child_stmt, 3, rec.content.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(upsert_child_stmt);
                sqlite3_reset(upsert_child_stmt);

                sqlite3_bind_int64(select_child_id_stmt, 1, current_parent_id);
                sqlite3_bind_text(select_child_id_stmt, 2, rec.content.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(select_child_id_stmt) == SQLITE_ROW) {
                    current_child_id = sqlite3_column_int64(select_child_id_stmt, 0);
                }
                sqlite3_reset(select_child_id_stmt);

            } else if (rec.type == "item") {
                if (current_child_id == -1) throw std::runtime_error("Hierarchical error: ITEM without CHILD at line " + std::to_string(rec.lineNumber));
                item_batch.push_back(rec);
                if (item_batch.size() >= BATCH_SIZE) {
                    flush_item_batch(current_child_id);
                }
            }
        }
        flush_item_batch(current_child_id);

    } catch (...) {
        // --- 4. 在异常情况下，确保所有语句都被终结 ---
        sqlite3_finalize(upsert_ym_stmt);
        sqlite3_finalize(select_ym_id_stmt);
        sqlite3_finalize(update_remark_stmt);
        sqlite3_finalize(upsert_parent_stmt);
        sqlite3_finalize(select_parent_id_stmt);
        sqlite3_finalize(upsert_child_stmt);
        sqlite3_finalize(select_child_id_stmt);
        sqlite3_finalize(insert_item_stmt);
        throw; // 重新抛出异常
    }

    // --- 5. 成功完成后，终结所有语句 ---
    sqlite3_finalize(upsert_ym_stmt);
    sqlite3_finalize(select_ym_id_stmt);
    sqlite3_finalize(update_remark_stmt);
    sqlite3_finalize(upsert_parent_stmt);
    sqlite3_finalize(select_parent_id_stmt);
    sqlite3_finalize(upsert_child_stmt);
    sqlite3_finalize(select_child_id_stmt);
    sqlite3_finalize(insert_item_stmt);
}