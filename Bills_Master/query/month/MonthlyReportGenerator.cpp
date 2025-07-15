// MonthlyReportGenerator.cpp
#include "MonthlyReportGenerator.h"
#include "month/_month_data/ReportData.h"

// 构造函数通过初始化列表来构建内部的 reader 和所有 formatters
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection), 
      m_markdown_formatter(), 
      m_latex_formatter(),
      m_typst_formatter() {} // 1. 在构造函数中初始化 typst_formatter

// generate 方法现在协调内部组件并根据所选格式进行操作
std::string MonthlyReportGenerator::generate(int year, int month, ReportFormat format) {
    // 步骤 1: 使用内部的 reader 从数据库读取和聚合数据
    MonthlyReportData data = m_reader.read_monthly_data(year, month);

    // 步骤 2: 根据传入的格式参数选择合适的格式化器
    std::string report;
    switch (format) {
        case ReportFormat::LATEX:
            report = m_latex_formatter.format_report(data);
            break;

        // 2. 为 TYPST 格式添加新的 case
        case ReportFormat::TYPST:
            report = m_typst_formatter.format_report(data);
            break;

        case ReportFormat::MARKDOWN:
        default: // 默认使用 Markdown 格式
            report = m_markdown_formatter.format_report(data);
            break;
    }

    // 步骤 3: 返回最终的报告
    return report;
}