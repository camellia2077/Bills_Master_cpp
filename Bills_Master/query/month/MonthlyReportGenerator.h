// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "query/month/month_format/ReportFormatterFactory.h" // 包含工厂头文件
#include "query/month/month_query/MonthQuery.h"             // 包含数据读取器头文件

class MonthlyReportGenerator {
public:
    /**
     * @brief 构造函数，初始化数据库读取器和插件工厂。
     */
    explicit MonthlyReportGenerator(sqlite3* db_connection);

    /**
     * @brief 公共接口：接收年份、月份和格式名称字符串，返回完整的报表。
     * @param year 要查询的年份。
     * @param month 要查询的月份。
     * @param format_name 报表的格式名称 (例如 "md", "tex", "typ")。
     * @return 包含所选格式报告的字符串。
     */
    std::string generate(int year, int month, const std::string& format_name);

private:
    MonthQuery m_reader;              // 职责1：负责读取数据
    ReportFormatterFactory m_factory; // 职责2：负责创建格式化器
};

#endif // MONTHLY_REPORT_GENERATOR_H