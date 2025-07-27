// YearMdFormat.h
#ifndef YEAR_MD_FORMAT_H
#define YEAR_MD_FORMAT_H

#include "query/year/year_format/IYearlyReportFormatter.h" // 包含接口头文件
#include "YearMdConfig.h" // 包含新的配置文件

class YearMdFormat : public IYearlyReportFormatter { // 继承接口
private:
    YearMdConfig config; // 持有一个配置对象

public:
    // 构造函数，允许传入一个自定义的配置对象，也支持使用默认配置
    explicit YearMdFormat(const YearMdConfig& config = YearMdConfig());

    // 使用 override 关键字确保它匹配基类函数
    std::string format_report(const YearlyReportData& data) const override;
};

#endif // YEAR_MD_FORMAT_H