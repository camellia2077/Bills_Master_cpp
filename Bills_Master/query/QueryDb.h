#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector> // Required for the new method
#include <sqlite3.h>

class QueryFacade {
public:
    explicit QueryFacade(const std::string& db_path);
    ~QueryFacade();

    // Disable copy and assignment as we manage a resource (the DB connection)
    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // --- Existing Methods ---
    std::string get_yearly_summary_report(const std::string& year);
    std::string get_monthly_details_report(const std::string& month);

    // --- New Method ---
    /**
     * @brief Retrieves all distinct bill dates (YYYYMM) from the database.
     * @return A vector of strings, where each string is a unique bill date.
     */
    std::vector<std::string> get_all_bill_dates();

private:
    sqlite3* m_db;
};

#endif // QUERY_FACADE_H
