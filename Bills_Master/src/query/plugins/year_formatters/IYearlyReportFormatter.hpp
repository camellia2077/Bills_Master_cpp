// query/year/year_format/IYearlyReportFormatter.hpp
#ifndef I_YEARLY_REPORT_FORMATTER_H
#define I_YEARLY_REPORT_FORMATTER_H

#include <string>
#include "query/components/yearly_report/YearlyReportData.hpp" // Path to the data structure

/**
 * @class IYearlyReportFormatter
 * @brief An interface (abstract base class) for report formatters.
 *
 * This defines a common interface that all concrete formatters must implement.
 * It allows client code to handle different report formats in a uniform way.
 */
class IYearlyReportFormatter {
public:
    // A virtual destructor is essential for a base class with virtual functions.
    virtual ~IYearlyReportFormatter() = default;

    /**
     * @brief Formats the yearly report data.
     * @param data The YearlyReportData struct containing all report data.
     * @return A formatted string representing the full report.
     */
    virtual std::string format_report(const YearlyReportData& data) const = 0; // Pure virtual function
};

#endif // I_YEARLY_REPORT_FORMATTER_H