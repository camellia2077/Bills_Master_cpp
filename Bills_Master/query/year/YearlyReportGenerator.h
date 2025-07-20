// YearlyReportGenerator.h
#ifndef YEARLY_REPORT_GENERATOR_H
#define YEARLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "ReportFormat.h"
#include "year_query/YearlyDataReader.h"
// The only include you need now is the factory
#include "year/year_format/YearlyReportFormatterFactory.h"

/**
 * @class YearlyReportGenerator
 * @brief A facade class that simplifies the report generation process.
 */
class YearlyReportGenerator {
public:
    explicit YearlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief The main public interface to generate a report.
     * @param year The year to query.
     * @param format The desired output format (defaults to Markdown).
     * @return A string containing the report in the requested format.
     */
    std::string generate(int year, ReportFormat format = ReportFormat::Markdown);

private:
    // You no longer need members for each formatter, only the data reader.
    YearlyDataReader m_reader;
};

#endif // YEARLY_REPORT_GENERATOR_H