#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <sqlite3.h>

class QueryFacade {
public:
    explicit QueryFacade(const std::string& db_path);
    ~QueryFacade();

    // 禁用拷贝和赋值
    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // --- 数据查询方法 ---
    std::string get_yearly_summary_report(int year);
    std::string get_monthly_details_report(int year, int month);
    std::vector<std::string> get_all_bill_dates();

    // --- 新增的报告导出方法 ---
    void export_yearly_report(const std::string& year_str, bool suppress_output = false);
    void export_monthly_report(const std::string& month_str, bool suppress_output = false);
    void export_all_reports();

private:
    sqlite3* m_db;
    void save_report(const std::string& report_content, const std::string& file_path_str);
};

#endif // QUERY_FACADE_H