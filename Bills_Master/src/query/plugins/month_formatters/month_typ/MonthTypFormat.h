// MonthTypFormat.h
#ifndef MONTH_TYP_FORMAT_H
#define MONTH_TYP_FORMAT_H

#include "query/plugins/month_formatters/IMonthReportFormatter.h"
#include "MonthTypConfig.h" // 包含新的配置头文件

class MonthTypFormat : public IMonthReportFormatter {
public:
    // 构造函数，通过依赖注入接收一个配置对象
    // 如果不提供，则会使用默认的配置
    explicit MonthTypFormat(const MonthTypConfig& config = MonthTypConfig{});

    // format_report 接口保持不变
    std::string format_report(const MonthlyReportData& data) const override;

private:
    std::string escape_typst(const std::string& input) const;

    // 持有一个配置对象的实例
    MonthTypConfig config_;
};

#endif // MONTH_TYP_FORMAT_H