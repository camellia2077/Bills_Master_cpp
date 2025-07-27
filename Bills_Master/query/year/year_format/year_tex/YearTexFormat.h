// YearTexFormat.h

#ifndef YEAR_TEX_FORMAT_H
#define YEAR_TEX_FORMAT_H

#include "query/year/year_format/IYearlyReportFormatter.h"
#include "YearlyTexReportConfig.h" // 1. 包含新的配置头文件

class YearTexFormat : public IYearlyReportFormatter {
public:
    // 2. 构造函数接收一个配置对象
    explicit YearTexFormat(const YearlyTexReportConfig& config = YearlyTexReportConfig{});

    std::string format_report(const YearlyReportData& data) const override;

private:
    // 3. 添加一个转义工具函数，以增强健壮性
    std::string escape_latex(const std::string& input) const;

    // 4. 持有一个配置对象的副本
    YearlyTexReportConfig m_config;
};

#endif // YEAR_TEX_FORMAT_H