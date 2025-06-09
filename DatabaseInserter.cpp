#include "DatabaseInserter.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

// Constructor: Opens the database connection.
DatabaseInserter::DatabaseInserter(const std::string& db_path) : db_path_(db_path) {
    if (sqlite3_open(db_path.c_str(), &db_)) {
        std::string errMsg = "Can't open database: ";
        errMsg += sqlite3_errmsg(db_);
        throw std::runtime_error(errMsg);
    }
}

// Destructor: Closes the database connection.
DatabaseInserter::~DatabaseInserter() {
    if (db_) {
        sqlite3_close(db_);
    }
}

// Helper to execute simple SQL statements.
void DatabaseInserter::execute_sql(const std::string& sql) {
    char* zErrMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), 0, 0, &zErrMsg) != SQLITE_OK) {
        std::string errMsg = "SQL error: " + std::string(zErrMsg);
        sqlite3_free(zErrMsg);
        throw std::runtime_error(errMsg);
    }
}

// Transaction management helpers.
void DatabaseInserter::begin_transaction() {
    execute_sql("BEGIN TRANSACTION;");
}

void DatabaseInserter::commit_transaction() {
    execute_sql("COMMIT;");
}

void DatabaseInserter::rollback_transaction() {
    // Use sqlite3_exec directly to avoid throwing an exception within a catch block.
    char* zErrMsg = nullptr;
    if (sqlite3_exec(db_, "ROLLBACK;", 0, 0, &zErrMsg) != SQLITE_OK) {
        std::cerr << "CRITICAL: Failed to rollback transaction: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
}

// Creates the database schema.
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
        throw; // Re-throw the exception after rolling back.
    }
}


// Inserts the parsed data stream.
// --- MODIFIED: REMOVED a top-level try-catch block and transaction management ---
void DatabaseInserter::insert_data_stream(const std::vector<ParsedRecord>& records) {
    sqlite3_stmt *stmt = nullptr;
    
    // Context IDs for tracking hierarchy
    long long current_year_month_id = -1;
    long long current_parent_id = -1;
    long long current_child_id = -1;

    // --- SQL Statements for Upserting and Querying IDs ---
    const char* upsert_year_month_sql = "INSERT INTO YearMonth(value) VALUES (?) ON CONFLICT(value) DO NOTHING;";
    const char* select_year_month_id_sql = "SELECT id FROM YearMonth WHERE value = ?;";

    const char* upsert_parent_sql = "INSERT INTO Parent(year_month_id, order_, title) VALUES (?, ?, ?) ON CONFLICT(year_month_id, title) DO UPDATE SET order_=excluded.order_;";
    const char* select_parent_id_sql = "SELECT id FROM Parent WHERE year_month_id = ? AND title = ?;";

    const char* upsert_child_sql = "INSERT INTO Child(parent_id, order_, title) VALUES (?, ?, ?) ON CONFLICT(parent_id, title) DO UPDATE SET order_=excluded.order_;";
    const char* select_child_id_sql = "SELECT id FROM Child WHERE parent_id = ? AND title = ?;";

    const char* insert_item_sql = "INSERT INTO Item(child_id, order_, amount, description) VALUES (?, ?, ?, ?);";
    
    // --- Batch processing for items ---
    const size_t BATCH_SIZE = 100;
    std::vector<ParsedRecord> item_batch;

    auto flush_item_batch = [&](long long child_id) {
        if (item_batch.empty()) return;
        
        sqlite3_stmt* item_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, insert_item_sql, -1, &item_stmt, nullptr) != SQLITE_OK) {
             throw std::runtime_error("Failed to prepare item insert statement: " + std::string(sqlite3_errmsg(db_)));
        }

        for(const auto& item_rec : item_batch) {
            sqlite3_bind_int64(item_stmt, 1, child_id);
            sqlite3_bind_int(item_stmt, 2, item_rec.order);
            sqlite3_bind_double(item_stmt, 3, item_rec.amount);
            sqlite3_bind_text(item_stmt, 4, item_rec.description.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(item_stmt) != SQLITE_DONE) {
                sqlite3_finalize(item_stmt);
                throw std::runtime_error("Failed to execute item insert statement: " + std::string(sqlite3_errmsg(db_)));
            }
            sqlite3_reset(item_stmt);
        }
        sqlite3_finalize(item_stmt);
        item_batch.clear();
    };


    // --- Main processing loop ---
    // NO LONGER HAS ITS OWN try/catch or begin/commit. The caller is responsible.
    for (const auto& rec : records) {
        if (rec.type == "date") {
            flush_item_batch(current_child_id); // Flush previous items before context changes
            current_parent_id = -1;
            current_child_id = -1;
            
            // Upsert YearMonth
            sqlite3_prepare_v2(db_, upsert_year_month_sql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, rec.content.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // Get ID
            sqlite3_prepare_v2(db_, select_year_month_id_sql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, rec.content.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                current_year_month_id = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);

        } else if (rec.type == "remark") {
            if (current_year_month_id == -1) throw std::runtime_error("Hierarchical error: REMARK found without a DATE context at line " + std::to_string(rec.lineNumber));
            const char* update_remark_sql = "UPDATE YearMonth SET remark = ? WHERE id = ?;";
            sqlite3_prepare_v2(db_, update_remark_sql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, rec.content.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 2, current_year_month_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
        } else if (rec.type == "parent") {
            flush_item_batch(current_child_id);
            current_child_id = -1;
            if (current_year_month_id == -1) throw std::runtime_error("Hierarchical error: PARENT found without a DATE context at line " + std::to_string(rec.lineNumber));
            
            // Upsert Parent
            sqlite3_prepare_v2(db_, upsert_parent_sql, -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, current_year_month_id);
            sqlite3_bind_int(stmt, 2, rec.order);
            sqlite3_bind_text(stmt, 3, rec.content.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // Get ID
            sqlite3_prepare_v2(db_, select_parent_id_sql, -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, current_year_month_id);
            sqlite3_bind_text(stmt, 2, rec.content.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                current_parent_id = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
            
        } else if (rec.type == "child") {
            flush_item_batch(current_child_id);
            if (current_parent_id == -1) throw std::runtime_error("Hierarchical error: CHILD found without a PARENT context at line " + std::to_string(rec.lineNumber));

            // Upsert Child
            sqlite3_prepare_v2(db_, upsert_child_sql, -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, current_parent_id);
            sqlite3_bind_int(stmt, 2, rec.order);
            sqlite3_bind_text(stmt, 3, rec.content.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            // Get ID
            sqlite3_prepare_v2(db_, select_child_id_sql, -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, current_parent_id);
            sqlite3_bind_text(stmt, 2, rec.content.c_str(), -1, SQLITE_STATIC);
             if (sqlite3_step(stmt) == SQLITE_ROW) {
                current_child_id = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);

        } else if (rec.type == "item") {
            if (current_child_id == -1) throw std::runtime_error("Hierarchical error: ITEM found without a CHILD context at line " + std::to_string(rec.lineNumber));
            item_batch.push_back(rec);
            if (item_batch.size() >= BATCH_SIZE) {
                flush_item_batch(current_child_id);
            }
        }
    }

    // Flush any remaining items in the batch
    flush_item_batch(current_child_id);
}