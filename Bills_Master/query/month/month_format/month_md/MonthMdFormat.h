// MonthMdFormat.h
#ifndef MD_FORMATT_H
#define MD_FORMATT_H

#include "month/month_format/IMonthReportFormatter.h" // 包含接口头文件
// 继承接口
class MonthMdFormat : public IMonthReportFormatter { 
public:
    // 使用 override 关键字
    std::string format_report(const MonthlyReportData& data) const override;
};

#endif // MD_FORMATT_H