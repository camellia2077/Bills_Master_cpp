#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "ReportFormat.h" // Include the shared enum

class QueryFacade {
public:
    explicit QueryFacade(const std::string& db_path);
    ~QueryFacade();

    // Disable copy and assignment
    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // --- Data Query Methods ---
    // UPDATED: Added format parameter
    std::string get_yearly_summary_report(int year, ReportFormat format = ReportFormat::MARKDOWN);
    std::string get_monthly_details_report(int year, int month, ReportFormat format = ReportFormat::MARKDOWN);
    std::vector<std::string> get_all_bill_dates();

    // --- Report Export Methods ---
    // UPDATED: Added format parameter
    void export_yearly_report(const std::string& year_str, ReportFormat format = ReportFormat::MARKDOWN, bool suppress_output = false);
    void export_monthly_report(const std::string& month_str, ReportFormat format = ReportFormat::MARKDOWN, bool suppress_output = false);
    void export_all_reports(ReportFormat format = ReportFormat::MARKDOWN);

private:
    sqlite3* m_db;
    void save_report(const std::string& report_content, const std::string& file_path_str);
};

#endif // QUERY_FACADE_H