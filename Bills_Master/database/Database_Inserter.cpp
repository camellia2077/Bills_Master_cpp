// Database_Inserter.cpp
#include "Database_Inserter.h"
#include <sqlite3.h>
#include <iostream>
#include <stdexcept>

// Constructor
DatabaseInserter::DatabaseInserter(const std::string& db_path) : db_path_(db_path) {
    if (sqlite3_open(db_path.c_str(), &db_)) {
        std::string errMsg = "Can't open database: ";
        errMsg += sqlite3_errmsg(db_);
        throw std::runtime_error(errMsg);
    }
}

// Destructor
DatabaseInserter::~DatabaseInserter() {
    if (db_) {
        sqlite3_close(db_);
    }
}

// Helper to execute simple SQL
void DatabaseInserter::execute_sql(const std::string& sql) {
    char* zErrMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), 0, 0, &zErrMsg) != SQLITE_OK) {
        std::string errMsg = "SQL error: " + std::string(zErrMsg);
        sqlite3_free(zErrMsg);
        throw std::runtime_error(errMsg);
    }
}

// Transaction controls
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

// Schema creation (unchanged)
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


// --- REWRITTEN and CORRECTED insert_data_stream function ---
void DatabaseInserter::insert_data_stream(const std::vector<ParsedRecord>& records) {
    if (records.empty()) {
        return;
    }

    // 1. Prepare all SQL statements once
    sqlite3_stmt *get_or_create_ym_stmt = nullptr;
    sqlite3_stmt *get_or_create_parent_stmt = nullptr;
    sqlite3_stmt *get_or_create_child_stmt = nullptr;
    sqlite3_stmt *insert_item_stmt = nullptr;

    const char* get_or_create_year_month_sql = "INSERT INTO YearMonth(value) VALUES (?) ON CONFLICT(value) DO UPDATE SET value=excluded.value RETURNING id;";
    const char* get_or_create_parent_sql = "INSERT INTO Parent(year_month_id, order_, title) VALUES (?, ?, ?) ON CONFLICT(year_month_id, title) DO UPDATE SET title=excluded.title RETURNING id;";
    const char* get_or_create_child_sql = "INSERT INTO Child(parent_id, order_, title) VALUES (?, ?, ?) ON CONFLICT(parent_id, title) DO UPDATE SET title=excluded.title RETURNING id;";
    const char* insert_item_sql = "INSERT INTO Item(child_id, order_, amount, description) VALUES (?, ?, ?, ?);"; 

    #define PREPARE_STMT(db, sql, stmt_ptr) \
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_ptr, nullptr) != SQLITE_OK) { \
            throw std::runtime_error("Failed to prepare statement (" + std::string(sql) + "): " + sqlite3_errmsg(db)); \
        }
    
    #define FINALIZE_STMT(stmt_ptr) if (stmt_ptr) sqlite3_finalize(stmt_ptr)

    try {
        PREPARE_STMT(db_, get_or_create_year_month_sql, get_or_create_ym_stmt);
        PREPARE_STMT(db_, get_or_create_parent_sql, get_or_create_parent_stmt);
        PREPARE_STMT(db_, get_or_create_child_sql, get_or_create_child_stmt);
        PREPARE_STMT(db_, insert_item_sql, insert_item_stmt);

        // State tracking to minimize DB lookups
        std::string last_date = "";
        std::string last_parent_cat = "";
        std::string last_child_cat = "";
        long long current_year_month_id = -1;
        long long current_parent_id = -1;
        long long current_child_id = -1;
        int parent_order = 0;
        int child_order = 0;
        int item_order = 0;

        for (const auto& rec : records) {
            // --- Step 1: Get/Create YearMonth ID ---
            if (rec.date != last_date) {
                sqlite3_bind_text(get_or_create_ym_stmt, 1, rec.date.c_str(), -1, SQLITE_STATIC);
                // sqlite3_step now returns SQLITE_ROW because of the RETURNING clause
                if (sqlite3_step(get_or_create_ym_stmt) == SQLITE_ROW) {
                    current_year_month_id = sqlite3_column_int64(get_or_create_ym_stmt, 0);
                } else {
                    throw std::runtime_error("Failed to get/create YearMonth ID for " + rec.date);
                }
                sqlite3_reset(get_or_create_ym_stmt);
                
                last_date = rec.date;
                last_parent_cat = "";
                parent_order = 0;
            }

            // --- Step 2: Get/Create Parent ID ---
            if (rec.parent_category != last_parent_cat) {
                parent_order++;
                sqlite3_bind_int64(get_or_create_parent_stmt, 1, current_year_month_id);
                sqlite3_bind_int(get_or_create_parent_stmt, 2, parent_order);
                sqlite3_bind_text(get_or_create_parent_stmt, 3, rec.parent_category.c_str(), -1, SQLITE_TRANSIENT);
                
                if (sqlite3_step(get_or_create_parent_stmt) == SQLITE_ROW) {
                    current_parent_id = sqlite3_column_int64(get_or_create_parent_stmt, 0);
                } else {
                    throw std::runtime_error("Failed to get/create Parent ID for " + rec.parent_category);
                }
                sqlite3_reset(get_or_create_parent_stmt);

                last_parent_cat = rec.parent_category;
                last_child_cat = "";
                child_order = 0;
            }

            // --- Step 3: Get/Create Child ID ---
            if (rec.child_category != last_child_cat) {
                child_order++;
                sqlite3_bind_int64(get_or_create_child_stmt, 1, current_parent_id);
                sqlite3_bind_int(get_or_create_child_stmt, 2, child_order);
                sqlite3_bind_text(get_or_create_child_stmt, 3, rec.child_category.c_str(), -1, SQLITE_TRANSIENT);
                
                if (sqlite3_step(get_or_create_child_stmt) == SQLITE_ROW) {
                    current_child_id = sqlite3_column_int64(get_or_create_child_stmt, 0);
                } else {
                    throw std::runtime_error("Failed to get/create Child ID for " + rec.child_category);
                }
                sqlite3_reset(get_or_create_child_stmt);
                
                last_child_cat = rec.child_category;
                item_order = 0;
            }
            
            // --- Step 4: Insert Item ---
            item_order++;
            sqlite3_bind_int64(insert_item_stmt, 1, current_child_id);
            sqlite3_bind_int(insert_item_stmt, 2, item_order);
            sqlite3_bind_double(insert_item_stmt, 3, rec.amount);
            sqlite3_bind_text(insert_item_stmt, 4, rec.item_description.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(insert_item_stmt) != SQLITE_DONE) throw std::runtime_error("Failed to insert Item");
            sqlite3_reset(insert_item_stmt);
        }

    } catch (...) {
        // In case of any exception, finalize all statements before re-throwing
        FINALIZE_STMT(get_or_create_ym_stmt);
        FINALIZE_STMT(get_or_create_parent_stmt);
        FINALIZE_STMT(get_or_create_child_stmt);
        FINALIZE_STMT(insert_item_stmt);
        throw;
    }

    // Finalize all statements on successful completion
    FINALIZE_STMT(get_or_create_ym_stmt);
    FINALIZE_STMT(get_or_create_parent_stmt);
    FINALIZE_STMT(get_or_create_child_stmt);
    FINALIZE_STMT(insert_item_stmt);
}