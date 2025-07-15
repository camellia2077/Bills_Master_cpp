// YearlyReportGenerator.cpp
#include "YearlyReportGenerator.h"
#include "year/_year_data/YearlyReportData.h" // 需要此定义来处理数据对象

// 构造函数初始化内部的 reader 和 formatter
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection), m_formatter() {}

// generate 方法协调内部组件来完成任务
std::string YearlyReportGenerator::generate(int year) {
    // 步骤 1: 使用 reader 获取数据
    YearlyReportData data = m_reader.read_yearly_data(year);

    // 步骤 2: 使用 formatter 格式化数据
    std::string report = m_formatter.format_report(data);

    // 步骤 3: 返回最终报告
    return report;
}