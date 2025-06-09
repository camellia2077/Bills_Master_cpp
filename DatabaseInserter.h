#ifndef DATABASE_INSERTER_H
#define DATABASE_INSERTER_H

#include <string>
#include <vector>
#include <stdexcept>
#include "BillParser.h" // Assuming BillParser.h defines ParsedRecord

// Forward declaration of the SQLite3 database handle
struct sqlite3;

/**
 * @class DatabaseInserter
 * @brief Handles insertion of parsed hierarchical bill data into an SQLite database.
 *
 * This class manages the database connection, schema creation, and transactional
 * insertion of records. It ensures data integrity through transactions and
 * hierarchical validation.
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
     * This method is idempotent.
     * @throws std::runtime_error on SQL execution failure.
     */
    void create_database();

    /**
     * @brief Inserts a stream of parsed records into the database.
     *
     * This method processes records sequentially, tracking the hierarchy. It uses
     * transactions to ensure atomicity. If any record is invalid or an SQL error
     * occurs, the entire transaction is rolled back. It uses an "upsert" strategy
     * to avoid duplicate entries.
     *
     * @param records A vector of ParsedRecord structs from the BillParser.
     * @throws std::runtime_error if hierarchical validation fails or an SQL error occurs.
     */
    void insert_data_stream(const std::vector<ParsedRecord>& records);

private:
    sqlite3* db_ = nullptr; // SQLite database handle
    std::string db_path_;

    // Private helper methods
    void execute_sql(const std::string& sql);
    void begin_transaction();
    void commit_transaction();
    void rollback_transaction();

    // Prevent copying and assignment
    DatabaseInserter(const DatabaseInserter&) = delete;
    DatabaseInserter& operator=(const DatabaseInserter&) = delete;
};

#endif // DATABASE_INSERTER_H