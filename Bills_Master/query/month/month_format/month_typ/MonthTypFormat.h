// MonthTypFormat.h
#ifndef MONTH_TYP_FORMAT_H
#define MONTH_TYP_FORMAT_H

#include "month/month_format/IMonthReportFormatter.h" // 包含接口头文件

class MonthTypFormat : public IMonthReportFormatter { // 继承接口
public:
    std::string format_report(const MonthlyReportData& data) const override;

private:
    std::string escape_typst(const std::string& input) const; // 建议设为 const
};

#endif // MONTH_TYP_FORMAT_H