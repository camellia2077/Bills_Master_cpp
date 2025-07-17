// MonthTexFormat.h
#ifndef TEX_FORMAT_H
#define TEX_FORMAT_H

#include "month/month_format/IMonthReportFormatter.h" // 包含接口头文件

class MonthTexFormat : public IMonthReportFormatter { // 继承接口
public:
    std::string format_report(const MonthlyReportData& data) const override;

private:
    std::string escape_latex(const std::string& input) const; 
};

#endif // TEX_FORMAT_H