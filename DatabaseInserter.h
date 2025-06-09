#ifndef DATABASE_INSERTER_H
#define DATABASE_INSERTER_H

// --- 添加以下缺失的头文件 ---
#include <string>
#include <vector>
#include <stdexcept>
#include "sqlite3.h"      // 用于 sqlite3 类型和相关函数
#include "Bill_Parser.h"   // 用于 ParsedRecord 结构体

/**
 * @class DatabaseInserter
 * @brief Handles insertion of parsed hierarchical bill data into an SQLite database.
 */
class DatabaseInserter {
public:
    /**
     * @brief Constructs a DatabaseInserter and opens a connection to the database.
     * @param db_path Filesystem path to the SQLite database file.
     * @throws std::runtime_error if the database connection cannot be established.
     */
    DatabaseInserter(const std::string& db_path);

    /**
     * @brief Destructor that closes the database connection.
     */
    ~DatabaseInserter();

    /**
     * @brief Creates the required tables and indexes in the database if they don't exist.
     */
    void create_database();

    /**
     * @brief Inserts a stream of parsed records into the database.
     * @param records A vector of ParsedRecord structs from the Bill_Parser.
     */
    void insert_data_stream(const std::vector<ParsedRecord>& records);

    // 事务控制函数需要是 public 的，以便 main 函数调用
    void begin_transaction();
    void commit_transaction();
    void rollback_transaction();

private:
    sqlite3* db_ = nullptr; // SQLite 数据库句柄
    std::string db_path_;

    // 私有辅助方法
    void execute_sql(const std::string& sql);

    // 禁止拷贝和赋值，确保数据库连接的唯一性
    DatabaseInserter(const DatabaseInserter&) = delete;
    DatabaseInserter& operator=(const DatabaseInserter&) = delete;
};

#endif // DATABASE_INSERTER_H