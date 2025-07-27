// YearTypFormat.h
#ifndef YEAR_TYP_FORMAT_H
#define YEAR_TYP_FORMAT_H

#include "query/year/year_format/IYearlyReportFormatter.h"
#include "YearlyTypConfig.h" // 包含新的配置文件

class YearTypFormat : public IYearlyReportFormatter {
public:
    // 构造函数，接收一个YearlyTypConfig对象。
    // 如果不提供，则使用默认配置。
    // 'explicit' 关键字防止意外的类型转换。
    explicit YearTypFormat(const YearlyTypConfig& config = YearlyTypConfig{});

    // format_report 接口保持不变
    std::string format_report(const YearlyReportData& data) const override;

private:
    // 在类内部持有一个配置对象的副本
    YearlyTypConfig config_; 
};

#endif // YEAR_TYP_FORMAT_H