// QueryDb.h
#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include "query/month/month_format/MonthPluginLoader.h"
#include "query/year/year_format/YearPluginLoader.h"

class QueryFacade {
public:
    explicit QueryFacade(
        const std::string& db_path, 
        const std::string& plugin_directory_path,
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );
    explicit QueryFacade(
        const std::string& db_path, 
        const std::vector<std::string>& plugin_paths,
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );
    ~QueryFacade();

    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // Report Generation API
    std::string get_yearly_summary_report(int year, const std::string& format_name);
    std::string get_monthly_details_report(int year, int month, const std::string& format_name);

    // Metadata API
    std::vector<std::string> get_all_bill_dates();

    // Report Export API
    bool export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output = false);
    
    // Unified Date Export API
    bool export_by_date(const std::string& date_str, const std::string& format_name);

    // --- 新增: 用于处理日期区间的导出接口 ---
    bool export_by_date_range(const std::string& start_date, const std::string& end_date, const std::string& format_name);

    // Batch Export API
    bool export_all_reports(const std::string& format_name);
    bool export_all_monthly_reports(const std::string& format_name);
    bool export_all_yearly_reports(const std::string& format_name);

private:
    sqlite3* m_db;
    std::string m_export_base_dir;
    std::map<std::string, std::string> m_format_folder_names;

    MonthPluginLoader m_month_manager;
    YearPluginLoader m_year_manager;

    void save_report(const std::string& report_content, const std::string& file_path_str);
    std::string get_display_format_name(const std::string& short_name) const;
};

#endif // QUERY_FACADE_H
