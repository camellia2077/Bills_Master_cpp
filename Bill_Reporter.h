#ifndef BILL_REPORTER_H
#define BILL_REPORTER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sqlite3.h>

// Forward-declare the main data structures to keep the header clean.
// These structures will hold the queried and sorted data.
struct ItemData;
struct ChildData;
struct ParentData;

// A vector is used to maintain the sort order from the SQL query.
using SortedData = std::vector<ParentData>;

/**
 * @class BillReporter
 * @brief Reads expense records from a SQLite database and generates analytical reports.
 */
class BillReporter {
public:
    /**
     * @brief Constructs a BillReporter object.
     * @param database_path The file path to the SQLite database.
     * @throws std::runtime_error if the database cannot be opened.
     */
    BillReporter(const std::string& database_path);

    /**
     * @brief Destructor that closes the database connection.
     */
    ~BillReporter();

    // BillReporter is non-copyable to prevent issues with the database handle.
    BillReporter(const BillReporter&) = delete;
    BillReporter& operator=(const BillReporter&) = delete;

    /**
     * @brief Query 1: Generates an annual consumption summary.
     * @param year The year to generate the report for (e.g., "2025").
     */
    void query_1(const std::string& year);

    /**
     * @brief Query 2: Generates a detailed, human-readable monthly bill.
     * @param year The year of the bill (e.g., "2025").
     * @param month The month of the bill (e.g., "01").
     */
    void query_2(const std::string& year, const std::string& month);

    /**
     * @brief Query 3: Exports monthly data in a machine-readable format.
     * @param year The year of the data (e.g., "2025").
     * @param month The month of the data (e.g., "01").
     */
    void query_3(const std::string& year, const std::string& month);

    /**
     * @brief Query 4: Finds the total spending for a specific category within a year.
     * @param year The year to search within (e.g., "2025").
     * @param parent_category The exact name of the parent category to sum up.
     */
    void query_4(const std::string& year, const std::string& parent_category);

private:
    sqlite3* db;
    std::string db_path;

    /**
     * @brief Core helper function to fetch and structure all data for a given month.
     * It retrieves items, groups them by sub- and parent-categories, and sorts
     * them according to the specified logic (by totals, descending).
     * @param year_month The month to query, in "YYYYMM" format.
     * @return A vector of ParentData structs, sorted and structured.
     */
    SortedData get_sorted_data(const std::string& year_month);
};

// --- Data Structure Definitions ---

/// @brief Represents a single expense item.
struct ItemData {
    double amount;
    std::string description;
};

/// @brief Represents a sub-category (e.g., "lunch", "subway").
struct ChildData {
    std::string title;
    double total;
    std::vector<ItemData> items; // Items within this sub-category
};

/// @brief Represents a parent category (e.g., "Food", "Transportation").
struct ParentData {
    std::string title;
    double total;
    std::vector<ChildData> children; // Sub-categories within this parent
};

#endif // BILL_REPORTER_H