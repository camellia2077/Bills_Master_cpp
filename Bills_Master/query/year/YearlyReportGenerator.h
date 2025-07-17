// YearlyReportGenerator.h
#ifndef YEARLY_REPORT_GENERATOR_H
#define YEARLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "ReportFormat.h"
#include "year_query/YearlyDataReader.h"
#include "year_format/YearMdFormat.h"
#include "year_format/YearTexFormat.h"
#include "year_format/YearTypFormat.h" 

/*
 * @class YearlyReportGenerator
 * @brief 封装年度报告生成流程的门面类。
 */
class YearlyReportGenerator {
public:
    // 构造函数接收一个数据库连接
    explicit YearlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief 公共接口：接收年份和格式，返回格式化后的年度报告字符串。
     * @param year 要查询的年份。
     * @param format 输出报告的格式 (默认为 MARKDOWN)。
     * @return 包含所选格式报告的字符串。
     */
    std::string generate(int year, ReportFormat format = ReportFormat::MARKDOWN);

private:
    // 内部持有的组件
    YearlyDataReader m_reader;
    YearMdFormat m_markdown_formatter;
    YearTexFormat m_latex_formatter;
    YearTypFormat m_typst_formatter; // 2. 添加 Typst 格式化器实例
};

#endif // YEARLY_REPORT_GENERATOR_H