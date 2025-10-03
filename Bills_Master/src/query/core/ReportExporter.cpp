// query/core/ReportExporter.cpp
#include "ReportExporter.hpp"
#include <fstream>
#include <stdexcept>
#include "common/common_utils.hpp" // For color definitions
#include <iostream>

ReportExporter::ReportExporter(const std::string& export_base_dir, const std::map<std::string, std::string>& format_folder_names)
    : m_export_base_dir(export_base_dir), m_format_folder_names(format_folder_names) {}

void ReportExporter::save_report(const std::string& report_content, const fs::path& file_path) {
    fs::create_directories(file_path.parent_path());
    std::ofstream output_file(file_path);
    if (!output_file) {
        throw std::runtime_error("Could not open file for writing: " + file_path.string());
    }
    output_file << report_content;
}

std::string ReportExporter::get_display_format_name(const std::string& short_name) const {
    if (short_name == "md") return "Markdown";
    if (short_name == "tex") return "LaTeX";
    if (short_name == "rst") return "reST";
    if (short_name == "typ") return "typst";
    return short_name;
}

void ReportExporter::export_yearly(const std::string& report_content, const std::string& year_str, const std::string& format_name) {
    if (report_content.find("未找到") != std::string::npos) {
        return; // Do not save "not found" reports
    }

    std::string extension = "." + format_name;
    std::string format_folder;
    auto it = m_format_folder_names.find(format_name);
    format_folder = (it != m_format_folder_names.end()) ? it->second : get_display_format_name(format_name) + "_bills";
    
    fs::path target_dir = m_export_base_dir / format_folder / "years";
    fs::path output_path = target_dir / (year_str + extension);
    
    save_report(report_content, output_path);
    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
}

void ReportExporter::export_monthly(const std::string& report_content, const std::string& month_str, const std::string& format_name) {
    if (report_content.find("未找到") != std::string::npos) {
        return; // Do not save "not found" reports
    }

    std::string extension = "." + format_name;
    std::string format_folder;
    auto it = m_format_folder_names.find(format_name);
    format_folder = (it != m_format_folder_names.end()) ? it->second : get_display_format_name(format_name) + "_bills";

    fs::path target_dir = m_export_base_dir / format_folder / "months" / month_str.substr(0, 4);
    fs::path output_path = target_dir / (month_str + extension);

    save_report(report_content, output_path);
    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
}