// reports/plugins/year_formatters/year_typ/YearTypFormat.cpp

#include "YearTypFormat.hpp"
#include <sstream>
#include <iomanip>

YearTypFormat::YearTypFormat(const YearlyTypConfig& config) : config_(config) {}

std::string YearTypFormat::get_no_data_message(int year) const {
    std::stringstream ss;
    ss << config_.labels.no_data_found_prefix << year << config_.labels.no_data_found_suffix << "\n";
    return ss.str();
}

std::string YearTypFormat::generate_header(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << "#set document(title: \"" << data.year << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";
    ss << "= " << data.year << config_.labels.year_suffix << config_.labels.report_title_suffix << "\n\n";
    return ss.str();
}

std::string YearTypFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config_.decimal_precision);
    ss << "== " << config_.labels.overview_section_title << "\n\n";
    ss << "  - *" << config_.labels.grand_total << ":* " << config_.currency_symbol << data.grand_total << "\n\n";
    return ss.str();
}

std::string YearTypFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    ss << "== " << config_.labels.monthly_breakdown_section_title << "\n\n";
    return ss.str();
}

std::string YearTypFormat::generate_monthly_item(int year, int month, double total) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config_.decimal_precision);
    ss << "  - " << year << "-" << std::setfill('0') << std::setw(2) << month
       << ":" << config_.currency_symbol << total << "\n";
    return ss.str();
}

std::string YearTypFormat::generate_footer(const YearlyReportData& data) const {
    return "";
}


// extern "C" 代码块保持不变
extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IYearlyReportFormatter* create_typ_year_formatter() {
        return new YearTypFormat();
    }
}