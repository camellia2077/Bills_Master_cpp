// QueryDb.h
#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <sqlite3.h>

class QueryFacade {
public:
    // Constructors remain the same
    explicit QueryFacade(const std::string& db_path, const std::string& plugin_directory_path);
    explicit QueryFacade(const std::string& db_path, const std::vector<std::string>& plugin_paths);
    ~QueryFacade();

    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // Public API for report generation
    std::string get_yearly_summary_report(int year, const std::string& format_name);
    std::string get_monthly_details_report(int year, int month, const std::string& format_name);

    // [FIXED] Add the missing method back to the public interface
    std::vector<std::string> get_all_bill_dates();

    // Export methods remain the same
    bool export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output = false);
    bool export_all_reports(const std::string& format_name);

private:
    sqlite3* m_db;
    std::string m_plugin_directory_path;
    std::vector<std::string> m_plugin_paths;
    bool m_use_plugin_list;

    void save_report(const std::string& report_content, const std::string& file_path_str);
    std::string get_display_format_name(const std::string& short_name) const;
};

#endif // QUERY_FACADE_H