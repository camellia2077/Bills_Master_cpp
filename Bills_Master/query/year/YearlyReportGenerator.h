// YearlyReportGenerator.h
#ifndef YEARLY_REPORT_GENERATOR_H
#define YEARLY_REPORT_GENERATOR_H

#include <string>
#include <sqlite3.h>
#include "_year_query/YearlyDataReader.h"   // 包含读取器
#include "_year_format/YearlyReportFormatter.h" // 包含格式化器

/*
 * @class YearlyReportGenerator
 * @brief 封装年度报告生成流程的门面类。
 *
 * 该类为用户提供一个简单的方法来获取年度报告，
 * 无需关心数据是如何从数据库读取或如何被格式化的。
 */
class YearlyReportGenerator {
public:
    // 构造函数接收一个数据库连接
    explicit YearlyReportGenerator(sqlite3* db_connection);

    // 公共接口：接收年份，返回格式化后的年度报告字符串
    std::string generate(int year);

private:
    // 内部持有的组件
    YearlyDataReader m_reader;
    YearlyReportFormatter m_formatter;
};

#endif // YEARLY_REPORT_GENERATOR_H