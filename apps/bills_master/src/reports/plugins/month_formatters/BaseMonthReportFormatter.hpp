// reports/plugins/month_formatters/BaseMonthReportFormatter.hpp
#ifndef BASE_MONTH_REPORT_FORMATTER_HPP
#define BASE_MONTH_REPORT_FORMATTER_HPP

#include "IMonthReportFormatter.hpp"
#include "reports/monthly_report/ReportSorter.hpp"
#include <string>
#include <sstream>
#include <vector>

// Forward declaration
struct MonthlyReportData;
struct ParentCategoryData;

class BaseMonthReportFormatter : public IMonthReportFormatter {
public:
    /**
     * @brief Final implementation that provides the overall structure for generating a report.
     *
     * This method handles common tasks like checking for data availability and sorting.
     * It then delegates the actual formatting of specific report sections to the
     * pure virtual functions that must be implemented by derived classes.
     */
    std::string format_report(const MonthlyReportData& data) const final;

    virtual ~BaseMonthReportFormatter() = default;

protected:
    // --- Functions to be implemented by derived classes ---

    /**
     * @brief Generates a message for when no data is found for the given month.
     * @param data The report data, containing year and month info.
     * @return A string with the "not found" message.
     */
    virtual std::string get_no_data_message(const MonthlyReportData& data) const = 0;

    /**
     * @brief Generates the header of the report (e.g., LaTeX preamble, Typst settings).
     * @param data The report data.
     * @return A string containing the report header.
     */
    virtual std::string generate_header(const MonthlyReportData& data) const = 0;

    /**
     * @brief Generates the summary section (total income, expense, balance, remarks).
     * @param data The report data.
     * @return A string containing the formatted summary.
     */
    virtual std::string generate_summary(const MonthlyReportData& data) const = 0;

    /**
     * @brief Generates the main body of the report, iterating through categories.
     * @param data The original report data.
     * @param sorted_parents The category data, pre-sorted by the base class.
     * @return A string containing the detailed breakdown of expenses.
     */
    virtual std::string generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const = 0;

    /**
     * @brief Generates the footer of the report (e.g., closing tags).
     * @param data The report data.
     * @return A string containing the report footer.
     */
    virtual std::string generate_footer(const MonthlyReportData& data) const = 0;
};

#endif // BASE_MONTH_REPORT_FORMATTER_HPP
