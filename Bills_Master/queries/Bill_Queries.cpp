#include "Bill_Queries.h"
#include <iostream>
#include <iomanip>
#include <numeric>
#include <vector>

// Constructor: Opens the database connection.
BillQueries::BillQueries(const std::string& database_path) : db(nullptr), db_path(database_path) {
    if (sqlite3_open(db_path.c_str(), &db)) {
        throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
    }
}

// Destructor: Closes the database connection.
BillQueries::~BillQueries() {
    if (db) {
        sqlite3_close(db);
    }
}

// Core Data Retrieval Function
SortedData BillQueries::get_sorted_data(const std::string& year_month) {
    // This SQL query uses Common Table Expressions (CTEs) to first calculate the total
    // for each parent and child category. It then joins everything to fetch all
    // required data in one go, pre-sorted by the database itself.
    const char* sql = R"(
        WITH parent_totals AS (
            SELECT p.id, SUM(i.amount) AS parent_total
            FROM Parent p
            JOIN Child c ON p.id = c.parent_id
            JOIN Item i ON c.id = i.child_id
            JOIN YearMonth ym ON p.year_month_id = ym.id
            WHERE ym.value = ?1 -- FIXED: Changed year_month to value
            GROUP BY p.id
        ),
        child_totals AS (
            SELECT c.id, SUM(i.amount) AS child_total
            FROM Child c
            JOIN Item i ON c.id = i.child_id
            JOIN Parent p ON c.parent_id = p.id
            JOIN YearMonth ym ON p.year_month_id = ym.id
            WHERE ym.value = ?1 -- FIXED: Changed year_month to value
            GROUP BY c.id
        )
        SELECT
            p.title AS parent_title,
            pt.parent_total,
            c.title AS child_title,
            ct.child_total,
            i.amount,
            i.description
        FROM Item i
        JOIN Child c ON i.child_id = c.id
        JOIN Parent p ON c.parent_id = p.id
        JOIN YearMonth ym ON p.year_month_id = ym.id
        JOIN parent_totals pt ON p.id = pt.id
        JOIN child_totals ct ON c.id = ct.id
        WHERE ym.value = ?1 -- FIXED: Changed year_month to value
        ORDER BY
            pt.parent_total DESC, p.title,
            ct.child_total DESC, c.title,
            i.amount DESC;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, year_month.c_str(), -1, SQLITE_STATIC);

    SortedData results;
    int res = sqlite3_step(stmt);
    while (res == SQLITE_ROW) {
        // Extract data from the current row
        std::string parent_title = (const char*)sqlite3_column_text(stmt, 0);
        double parent_total = sqlite3_column_double(stmt, 1);
        std::string child_title = (const char*)sqlite3_column_text(stmt, 2);
        double child_total = sqlite3_column_double(stmt, 3);
        double item_amount = sqlite3_column_double(stmt, 4);
        std::string item_desc = (const char*)sqlite3_column_text(stmt, 5);

        // Because the data is sorted, we can build the nested structure sequentially.
        // Find or create the ParentData struct.
        if (results.empty() || results.back().title != parent_title) {
            results.push_back({parent_title, parent_total, {}});
        }
        
        // Find or create the ChildData struct within the current parent.
        auto& children = results.back().children;
        if (children.empty() || children.back().title != child_title) {
            children.push_back({child_title, child_total, {}});
        }
        
        // Add the item to the current child.
        children.back().items.push_back({item_amount, item_desc});
        
        res = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return results;
}


// Query 1: Annual Summary
void BillQueries::query_1(const std::string& year) {
    const char* sql = R"(
        SELECT
            substr(ym.value, 5, 2) as month, -- FIXED: Changed year_month to value
            SUM(i.amount) as monthly_total
        FROM Item i
        JOIN Child c ON i.child_id = c.id
        JOIN Parent p ON c.parent_id = p.id
        JOIN YearMonth ym ON p.year_month_id = ym.id
        WHERE substr(ym.value, 1, 4) = ? -- FIXED: Changed year_month to value
        GROUP BY month ORDER BY month;
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("query_1: Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, year.c_str(), -1, SQLITE_STATIC);

    std::vector<std::pair<std::string, double>> monthly_totals;
    double total_consumption = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string month = (const char*)sqlite3_column_text(stmt, 0);
        double total = sqlite3_column_double(stmt, 1);
        monthly_totals.push_back({month, total});
        total_consumption += total;
    }
    sqlite3_finalize(stmt);

    if (monthly_totals.empty()) {
        std::cout << "无数据" << std::endl;
        return;
    }

    double average_monthly = total_consumption / monthly_totals.size();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "年度总消费: " << total_consumption << std::endl;
    std::cout << "月均消费: " << average_monthly << std::endl;
    std::cout << "各月消费明细:" << std::endl;
    for (const auto& pair : monthly_totals) {
        std::cout << "  - " << year << "年" << pair.first << "月: " << pair.second << std::endl;
    }
}


// Query 2: Detailed Monthly Bill
void BillQueries::query_2(const std::string& year, const std::string& month) {
    std::string year_month = year + month;
    SortedData data = get_sorted_data(year_month);

    if (data.empty()) {
        std::cout << "无数据" << std::endl;
        return;
    }

    double grand_total = 0;
    for (const auto& parent : data) {
        grand_total += parent.total;
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "--- " << year << "年" << month << "月 账单详情 ---\n";
    std::cout << "总消费: " << grand_total << "\n\n";

    for (const auto& parent : data) {
        double percentage = (grand_total > 0) ? (parent.total / grand_total * 100.0) : 0.0;
        std::cout << parent.title << " (总计: " << parent.total << ", 占比: " << percentage << "%)" << std::endl;
        for (const auto& child : parent.children) {
            std::cout << "  - " << child.title << " (总计: " << child.total << ")" << std::endl;
            for (const auto& item : child.items) {
                std::cout << "    - " << item.amount << " " << item.description << std::endl;
            }
        }
        std::cout << std::endl; // Blank line between parent categories
    }
}


// Query 3: Machine-Readable Export
void BillQueries::query_3(const std::string& year, const std::string& month) {
    std::string year_month = year + month;
    SortedData data = get_sorted_data(year_month);

    if (data.empty()) {
        std::cout << "无数据" << std::endl;
        return;
    }

    std::cout << "DATE" << year_month << std::endl;
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) {
            std::cout << std::endl; // Blank line between parent blocks
        }
        const auto& parent = data[i];
        std::cout << parent.title << std::endl;
        for (const auto& child : parent.children) {
            std::cout << child.title << std::endl;
            for (const auto& item : child.items) {
                std::cout << item.amount << item.description << std::endl;
            }
        }
    }
}


// Query 4: Annual Category Total
void BillQueries::query_4(const std::string& year, const std::string& parent_category) {
    const char* sql = R"(
        SELECT SUM(i.amount)
        FROM Item i
        JOIN Child c ON i.child_id = c.id
        JOIN Parent p ON c.parent_id = p.id
        JOIN YearMonth ym ON p.year_month_id = ym.id
        WHERE substr(ym.value, 1, 4) = ?1 AND p.title = ?2; -- FIXED: Changed year_month to value
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("query_4: Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    sqlite3_bind_text(stmt, 1, year.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, parent_category.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // Check if the result is NULL (happens if no rows match)
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            double total = sqlite3_column_double(stmt, 0);
            std::cout << std::fixed << std::setprecision(2);
            std::cout << year << "年[" << parent_category << "]总消费: " << total << "元" << std::endl;
        } else {
            std::cout << "无数据" << std::endl;
        }
    } else {
        std::cout << "无数据" << std::endl;
    }
    sqlite3_finalize(stmt);
}