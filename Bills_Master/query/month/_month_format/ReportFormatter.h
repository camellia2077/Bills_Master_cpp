// ReportFormatter.h
#ifndef REPORT_FORMATTER_H
#define REPORT_FORMATTER_H

#include <string>
#include "month/_month_data/ReportData.h"

class ReportFormatter {
public:
    // 接收数据结构并返回格式化的字符串
    std::string format_report(const MonthlyReportData& data);
};

#endif // REPORT_FORMATTER_H