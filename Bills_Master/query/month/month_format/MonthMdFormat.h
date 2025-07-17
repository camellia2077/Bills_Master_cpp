// MonthMdFormat.h
#ifndef MD_FORMATT_H
#define MD_FORMATT_H

#include <string>
#include "month/_month_data/ReportData.h"

class MonthMdFormat {
public:
    // 接收数据结构并返回格式化的字符串
    std::string format_report(const MonthlyReportData& data);
};

#endif // MD_FORMATT_H