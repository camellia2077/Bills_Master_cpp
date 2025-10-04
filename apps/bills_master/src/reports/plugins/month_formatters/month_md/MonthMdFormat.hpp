// reports/plugins/month_formatters/month_md/MonthMdFormat.hpp
#ifndef MONTH_MD_FORMAT_HPP
#define MONTH_MD_FORMAT_HPP

#include "reports/plugins/month_formatters/IMonthReportFormatter.hpp" // 包含接口头文件
#include "MonthMdConfig.hpp" // 包含新的配置文件

// 继承接口
class MonthMdFormat : public IMonthReportFormatter {
private:
    MonthMdConfig config; // 持有一个配置对象

public:
    // 构造函数，允许传入一个自定义的配置对象，也支持使用默认配置
    explicit MonthMdFormat(const MonthMdConfig& config = MonthMdConfig());

    // 使用 override 关键字实现接口
    std::string format_report(const MonthlyReportData& data) const override;
};

#endif // MONTH_MD_FORMAT_HPP