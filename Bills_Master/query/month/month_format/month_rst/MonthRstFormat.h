// MonthRstFormat.h
#ifndef MONTH_RST_FORMAT_H
#define MONTH_RST_FORMAT_H

#include "query/month/month_format/IMonthReportFormatter.h" // 包含基类接口

// 继承自 IMonthReportFormatter 接口
class MonthRstFormat : public IMonthReportFormatter {
public:
    // 实现接口中定义的纯虚函数
    std::string format_report(const MonthlyReportData& data) const override;
};

#endif // MONTH_RST_FORMAT_H