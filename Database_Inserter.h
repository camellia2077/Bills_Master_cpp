// Database_Inserter.h
#ifndef DATABASE_INSERTER_H
#define DATABASE_INSERTER_H

#include <string>
#include <vector>
#include <stdexcept>
#include <sqlite3.h>
#include "Bill_Parser.h" // Ensures ParsedRecord is available

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

    // Transaction control functions
    void begin_transaction();
    void commit_transaction();
    void rollback_transaction();

private:
    sqlite3* db_ = nullptr;
    std::string db_path_;

    void execute_sql(const std::string& sql);

    // Disable copying and assignment to ensure a single database connection.
    DatabaseInserter(const DatabaseInserter&) = delete;
    DatabaseInserter& operator=(const DatabaseInserter&) = delete;
};

#endif // DATABASE_INSERTER_H