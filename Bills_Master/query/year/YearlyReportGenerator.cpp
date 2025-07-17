// YearlyReportGenerator.cpp
#include "YearlyReportGenerator.h"
#include "year/_year_data/YearlyReportData.h"

// 构造函数初始化内部的 reader 和所有 formatters
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection), 
      m_markdown_formatter(), 
      m_latex_formatter(),
      m_typst_formatter(),
      m_rst_formatter()
       {} 

// generate 方法现在根据所选格式进行操作
std::string YearlyReportGenerator::generate(int year, ReportFormat format) {
    // 步骤 1: 使用 reader 获取数据
    YearlyReportData data = m_reader.read_yearly_data(year);

    // 步骤 2: 根据传入的格式参数选择合适的格式化器
    std::string report;
    switch (format) {
        case ReportFormat::LaTeX:
            report = m_latex_formatter.format_report(data);
            break;
        case ReportFormat::Typst:
            report = m_typst_formatter.format_report(data);
            break;
        case ReportFormat::Rst:
            report = m_rst_formatter.format_report(data);
            break;
        case ReportFormat::Markdown:
        default: // 默认使用 Markdown 格式
            report = m_markdown_formatter.format_report(data);
            break;
    }

    // 步骤 3: 返回最终报告
    return report;
}