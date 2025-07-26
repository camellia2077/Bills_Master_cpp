#ifndef REPORT_EXPORTER_H
#define REPORT_EXPORTER_H

#include <string>
#include "QueryDb.h"
#include "app_controller/ProcessStats.h"

class ReportExporter {
public:
    explicit ReportExporter(const std::string& db_path);

    void export_all_reports();
    bool export_yearly_report(const std::string& year_str, bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, bool suppress_output = false);

private:
    QueryFacade m_query_facade;
    std::string m_db_path;

    void save_report(const std::string& report_content, const std::string& file_path);
};

#endif // REPORT_EXPORTER_H