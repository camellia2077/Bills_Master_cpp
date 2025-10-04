// reports/core/ReportExporter.hpp
#ifndef REPORT_EXPORTER_HPP
#define REPORT_EXPORTER_HPP

#include <string>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

class ReportExporter {
public:
    explicit ReportExporter(
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );

    void export_yearly(const std::string& report_content, const std::string& year_str, const std::string& format_name);
    void export_monthly(const std::string& report_content, const std::string& month_str, const std::string& format_name);

private:
    fs::path m_export_base_dir;
    std::map<std::string, std::string> m_format_folder_names;

    void save_report(const std::string& report_content, const fs::path& file_path);
    std::string get_display_format_name(const std::string& short_name) const;
};

#endif // REPORT_EXPORTER_HPP