// QueryDb.h
#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <sqlite3.h>
// #include "ReportFormat.h" // 这一行应该已经被移除了

class QueryFacade {
public:
    explicit QueryFacade(const std::string& db_path);
    ~QueryFacade();

    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // 公共接口保持不变
    std::string get_yearly_summary_report(int year, const std::string& format_name);
    std::string get_monthly_details_report(int year, int month, const std::string& format_name);
    std::vector<std::string> get_all_bill_dates();

    // 将 void 修改为 bool
    bool export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output = false);
    bool export_all_reports(const std::string& format_name);

private:
    sqlite3* m_db;
    void save_report(const std::string& report_content, const std::string& file_path_str);

    // ======================================================================
    // ==                    核心修改点 START                            ==
    // ==  新增一个私有辅助函数，用于将短格式名转换为统一的目录显示名       ==
    // ======================================================================
    std::string get_display_format_name(const std::string& short_name) const;
    // ======================================================================
    // ==                     核心修改点 END                             ==
    // ======================================================================
};

#endif // QUERY_FACADE_H