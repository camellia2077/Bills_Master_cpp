#include "common/pch.h"
#include "ExportController.h"
#include "query/core/QueryDb.h"
#include "common/common_utils.h" // 确保包含颜色定义

#include <iostream>
#include <stdexcept>

ExportController::ExportController(const std::string& db_path,
                                   const std::vector<std::string>& plugin_files,
                                   const std::string& export_base_dir,
                                   const std::map<std::string, std::string>& format_folder_names)
    : m_db_path(db_path),
      m_plugin_files(plugin_files),
      m_export_base_dir(export_base_dir),
      m_format_folder_names(format_folder_names) {}

// handle_export 函数的实现代码与 AppController.cpp 中的完全相同，直接复制过来即可。
bool ExportController::handle_export(const std::string& type, const std::vector<std::string>& values, const std::string& format_str) {
    bool success = false;
    try {
        QueryFacade facade(m_db_path, m_plugin_files, m_export_base_dir, m_format_folder_names);

        if (type == "all") {
            success = facade.export_all_reports(format_str);
        } else if (type == "all_months") {
            success = facade.export_all_monthly_reports(format_str);
        } else if (type == "all_years") {
            success = facade.export_all_yearly_reports(format_str);
        } else if (type == "date") {
            if (values.empty()) {
                throw std::runtime_error("At least one date string must be provided for 'date' export.");
            }
            if (values.size() == 1) { // 单个日期
                success = facade.export_by_date(values[0], format_str);
            } else if (values.size() == 2) { // 日期区间
                success = facade.export_by_date_range(values[0], values[1], format_str);
            } else {
                throw std::runtime_error("For 'date' export, please provide one (YYYY or YYYYMM) or two (YYYYMM YYYYMM) date values.");
            }
        } else if (type == "year") {
            if (values.empty() || values[0].empty()) {
                throw std::runtime_error("A year must be provided to export a yearly report.");
            }
            success = facade.export_yearly_report(values[0], format_str);
        } else if (type == "month") {
            if (values.empty() || values[0].empty()) {
                throw std::runtime_error("A month (YYYYMM) must be provided to export a monthly report.");
            }
            success = facade.export_monthly_report(values[0], format_str);
        } else {
            throw std::runtime_error("Unknown export type: " + type);
        }
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "Export failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
    return success;
}