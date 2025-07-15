// MonthlyReportGenerator.cpp
#include "MonthlyReportGenerator.h"
#include "month/_month_data/ReportData.h" // 需要此头文件来获取 MonthlyReportData 类型

// 构造函数通过初始化列表来构建内部的 reader 和 formatter
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection), m_formatter() {}

// generate 方法协调内部组件来完成任务
std::string MonthlyReportGenerator::generate(int year, int month) {
    // 步骤 1: 使用内部的 reader 从数据库读取和聚合数据
    MonthlyReportData data = m_reader.read_monthly_data(year, month);

    // 步骤 2: 使用内部的 formatter 将数据结构格式化为字符串
    std::string report = m_formatter.format_report(data);

    // 步骤 3: 返回最终的报告
    return report;
}