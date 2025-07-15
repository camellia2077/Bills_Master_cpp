// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "ReportFormat.h" // USE THE SHARED HEADER
#include "_month_query/TransactionReader.h"
#include "_month_format/ReportFormatter.h"
#include "_month_format/LatexReportFormatter.h"

// The duplicate enum definition that was here has been REMOVED.

/*
 * @class MonthlyReportGenerator
 * @brief A facade class that encapsulates the entire report generation process.
 *
 * This class hides the internal complexity of data reading and formatting,
 * providing the user with a simple method to get the final monthly report.
 */
class MonthlyReportGenerator {
public:
    // Constructor receives a database connection
    explicit MonthlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief Public interface: receives year, month, and format, returns the complete report string.
     * @param year The year to query.
     * @param month The month to query.
     * @param format The format of the output report (defaults to MARKDOWN).
     * @return A string containing the report in the selected format.
     */
    std::string generate(int year, int month, ReportFormat format = ReportFormat::MARKDOWN);

private:
    // Internally held components, hidden from the caller
    TransactionReader m_reader;
    ReportFormatter m_markdown_formatter;
    LatexReportFormatter m_latex_formatter;
};

#endif // MONTHLY_REPORT_GENERATOR_H