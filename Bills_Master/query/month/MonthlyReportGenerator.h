// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "query/month/month_format/ReportFormatterFactory.h" // Use the factory instead of individual formatters
#include "query/month/month_query/MonthQuery.h"

/**
 * @class MonthlyReportGenerator
 * @brief 一个门面类，利用工厂模式封装了报表生成的整个流程。
 *
 * The class now uses ReportFormatterFactory to dynamically create the
 * required formatter, instead of holding an instance of each one.
 */
class MonthlyReportGenerator {
public:
    // The constructor now only needs to initialize the query component.
    explicit MonthlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief 公共接口：接收年份、月份和格式，返回完整的报表字符串。
     * @param year The year to query.
     * @param month The month to query.
     * @param format The format of the output report (defaults to MARKDOWN).
     * @return A string containing the report in the selected format.
     */
    std::string generate(int year, int month, ReportFormat format = ReportFormat::Markdown);

private:
    // Only the query component is needed as a member.
    MonthQuery m_reader;
};

#endif // MONTHLY_REPORT_GENERATOR_H