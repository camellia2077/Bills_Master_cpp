// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "ReportFormat.h"
#include "_month_query/TransactionReader.h"
#include "_month_format/ReportFormatter.h"
#include "_month_format/LatexReportFormatter.h"
#include "_month_format/TypstReportFormatter.h" // 1. 包含新的 Typst 格式化器

/*
 * @class MonthlyReportGenerator
 * @brief 一个门面类，封装了报表生成的整个流程。
 */
class MonthlyReportGenerator {
public:
    // 构造函数接收一个数据库连接
    explicit MonthlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief 公共接口：接收年份、月份和格式，返回完整的报表字符串。
     * @param year The year to query.
     * @param month The month to query.
     * @param format The format of the output report (defaults to MARKDOWN).
     * @return A string containing the report in the selected format.
     */
    std::string generate(int year, int month, ReportFormat format = ReportFormat::MARKDOWN);

private:
    // 内部持有的组件
    TransactionReader m_reader;
    ReportFormatter m_markdown_formatter;
    LatexReportFormatter m_latex_formatter;
    TypstReportFormatter m_typst_formatter; // 2. 添加 Typst 格式化器实例
};

#endif // MONTHLY_REPORT_GENERATOR_H