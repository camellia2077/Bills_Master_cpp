#ifndef REPORT_EXPORTER_H
#define REPORT_EXPORTER_H

#include <string>
#include "query/QueryDb.h"
#include "app_controller/ProcessStats.h"

class ReportExporter {
public:
    explicit ReportExporter(const std::string& db_path);

    // =======================================================================
    // ==                      核心修改点 START                            ==
    // ==  为所有公共接口添加 format_name 参数，并提供默认值 "md"            ==
    // =======================================================================
    bool export_all_reports(const std::string& format_name = "md"); // 将 void 修改为 bool
    bool export_yearly_report(const std::string& year_str, const std::string& format_name = "md", bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, const std::string& format_name = "md", bool suppress_output = false);
    // =======================================================================
    // ==                       核心修改点 END                             ==
    // =======================================================================

private:
    QueryFacade m_query_facade;
    std::string m_db_path;

    void save_report(const std::string& report_content, const std::string& file_path);
};

#endif // REPORT_EXPORTER_H 