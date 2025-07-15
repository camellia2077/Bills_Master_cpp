#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "month/MonthlyReportGenerator.h" // 1. 包含以获取 ReportFormat 枚举

class QueryFacade {
public:
    explicit QueryFacade(const std::string& db_path);
    ~QueryFacade();

    // 禁用拷贝和赋值
    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // --- 数据查询方法 ---
    std::string get_yearly_summary_report(int year);
    // 2. 更新 get_monthly_details_report 以接受格式参数
    std::string get_monthly_details_report(int year, int month, ReportFormat format = ReportFormat::MARKDOWN);
    std::vector<std::string> get_all_bill_dates();

    // --- 报告导出方法 ---
    void export_yearly_report(const std::string& year_str, bool suppress_output = false);
    // 3. 更新 export_monthly_report 以接受格式参数
    void export_monthly_report(const std::string& month_str, ReportFormat format = ReportFormat::MARKDOWN, bool suppress_output = false);
    // 4. 更新 export_all_reports 以接受格式参数
    void export_all_reports(ReportFormat format = ReportFormat::MARKDOWN);

private:
    sqlite3* m_db;
    void save_report(const std::string& report_content, const std::string& file_path_str);
};

#endif // QUERY_FACADE_H