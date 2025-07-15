// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "_month_query/TransactionReader.h" // 包含读取器依赖
#include "_month_format/ReportFormatter.h"   // 包含格式化器依赖

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

    // 公共接口：接收年份和月份，返回完整的报表字符串
    std::string generate(int year, int month);

private:
    // 内部持有的组件，对调用者隐藏
    TransactionReader m_reader;
    ReportFormatter m_formatter;
};

#endif // MONTHLY_REPORT_GENERATOR_H