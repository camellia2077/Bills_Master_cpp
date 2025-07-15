// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "_month_query/TransactionReader.h"
#include "_month_format/ReportFormatter.h"
#include "_month_format/LatexReportFormatter.h" // 1. 包含新的 LaTeX 格式化器

/**
 * @enum ReportFormat
 * @brief 定义报告的输出格式类型。
 */
enum class ReportFormat {
    MARKDOWN, // 默认的 Markdown 格式
    LATEX     // 新增的 LaTeX 格式
};

/*
 * @class MonthlyReportGenerator
 * @brief 一个门面类，封装了报表生成的整个流程。
 *
 * 该类隐藏了数据读取和格式化的内部复杂性，
 * 为用户提供一个简单的方法来获取最终的月度报告。
 */
class MonthlyReportGenerator {
public:
    // 构造函数接收一个数据库连接
    explicit MonthlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief 公共接口：接收年份、月份和格式，返回完整的报表字符串。
     * @param year 要查询的年份。
     * @param month 要查询的月份。
     * @param format 输出报告的格式 (默认为 MARKDOWN)。
     * @return 包含所选格式报告的字符串。
     */
    std::string generate(int year, int month, ReportFormat format = ReportFormat::MARKDOWN); // 2. 修改 generate 方法

private:
    // 内部持有的组件，对调用者隐藏
    TransactionReader m_reader;
    ReportFormatter m_markdown_formatter;   // 3. 重命名以明确其用途
    LatexReportFormatter m_latex_formatter; // 4. 添加 LaTeX 格式化器实例
};

#endif // MONTHLY_REPORT_GENERATOR_H