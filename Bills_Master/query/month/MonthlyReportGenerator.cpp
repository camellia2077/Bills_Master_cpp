// MonthlyReportGenerator.cpp
#include "MonthlyReportGenerator.h"
#include "query/month/_month_data/ReportData.h"
#include "query/month/month_format/IMonthReportFormatter.h"
#include <stdexcept>

// 在构造函数的初始化列表中初始化工厂
// 默认在 "plugins" 目录下查找插件
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection), m_factory("plugins") {
}

// generate方法的实现
std::string MonthlyReportGenerator::generate(int year, int month, const std::string& format_name) {
    // 步骤 1: 使用内部读取器从数据库获取数据
    MonthlyReportData data = m_reader.read_monthly_data(year, month);

    // 步骤 2: 将格式名称字符串传递给工厂，让工厂创建对应的格式化器实例
    auto formatter = m_factory.createFormatter(format_name);

    // 步骤 3: 如果工厂返回空指针，说明没有找到对应的插件
    if (!formatter) {
        throw std::runtime_error("Unsupported or unloaded report format specified: " + format_name);
    }

    // 步骤 4: 使用创建好的格式化器生成报告并返回
    return formatter->format_report(data);
}