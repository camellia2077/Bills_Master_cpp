// query/core/QueryFacade.hpp
#ifndef QUERY_FACADE_HPP
#define QUERY_FACADE_HPP

#include "query/components/monthly_report/MonthlyReportGenerator.hpp"
#include "query/components/yearly_report/YearlyReportGenerator.hpp"
#include "query/core/ReportExporter.hpp"
#include "query/core/BillMetadataReader.hpp"

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class QueryFacade {
public:
    // Constructor for loading plugins from a directory
    explicit QueryFacade(
        const std::string& db_path, 
        const std::string& plugin_directory_path,
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );
    
    // Constructor for loading plugins from a list of files
    explicit QueryFacade(
        const std::string& db_path, 
        const std::vector<std::string>& plugin_paths,
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );
    
    ~QueryFacade();

    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // Report Export APIs
    bool export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output = false);
    bool export_by_date(const std::string& date_str, const std::string& format_name);
    bool export_by_date_range(const std::string& start_date, const std::string& end_date, const std::string& format_name);

    // Batch Export APIs
    bool export_all_reports(const std::string& format_name);
    bool export_all_monthly_reports(const std::string& format_name);
    bool export_all_yearly_reports(const std::string& format_name);

private:
    sqlite3* m_db;
    std::unique_ptr<MonthlyReportGenerator> m_monthly_generator;
    std::unique_ptr<YearlyReportGenerator> m_yearly_generator;
    std::unique_ptr<ReportExporter> m_report_exporter;
    std::unique_ptr<BillMetadataReader> m_metadata_reader;
};

#endif // QUERY_FACADE_HPP