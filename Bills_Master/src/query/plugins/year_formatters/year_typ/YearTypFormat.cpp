#include "common/pch.h"
#include "YearTypFormat.h"
#include <sstream>
#include <iomanip>

// The constructor initializes the configuration object.
YearTypFormat::YearTypFormat(const YearlyTypConfig& config) : config_(config) {}

std::string YearTypFormat::format_report(const YearlyReportData& data) const {
    std::stringstream ss;

    // --- Document header setup (from config) ---
    ss << "#set document(title: \"" << data.year << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";
    
    // --- Report body (from config) ---
    ss << "= " << data.year << config_.labels.year_suffix << config_.labels.report_title_suffix << "\n\n";

    if (!data.data_found) {
        ss << config_.labels.no_data_found_prefix << data.year << config_.labels.no_data_found_suffix << "\n";
        return ss.str();
    }
    
    ss << std::fixed << std::setprecision(config_.decimal_precision);

    // --- Summary section (from config) ---
    ss << "== " << config_.labels.overview_section_title << "\n\n";
    ss << "  - *" << config_.labels.grand_total << ":* " << config_.currency_symbol << data.grand_total << "\n\n";
    
    // --- Monthly breakdown (from config) ---
    ss << "== " << config_.labels.monthly_breakdown_section_title << "\n\n";
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "  - " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val
           << ":" << config_.currency_symbol << month_total << "\n";
    }

    return ss.str();
}

// =================================================================
// ================== 以下是为动态库添加的部分 ==================
// =================================================================
extern "C" {

    // Define platform-specific export macros
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    /**
     * @brief Creates an instance of the YearTypFormat formatter.
     * @return A pointer to the IYearlyReportFormatter interface.
     *
     * This is the sole entry point for this dynamic library. The main application
     * will load the library and call this function to get a formatter object.
     */
    PLUGIN_API IYearlyReportFormatter* create_typ_year_formatter() {
        // Create and return a new formatter instance
        return new YearTypFormat();
    }

} // extern "C"