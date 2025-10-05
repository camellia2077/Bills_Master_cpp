// reports/plugins/month_formatters/month_rst/MonthRstFormat.hpp
#ifndef MONTH_RST_FORMAT_HPP
#define MONTH_RST_FORMAT_HPP

#include "reports/plugins/month_formatters/IMonthReportFormatter.hpp" // 包含基类接口
#include "MonthRstConfig.hpp" // 包含新的配置文件

// 继承自 IMonthReportFormatter 接口
class MonthRstFormat : public IMonthReportFormatter {
private:
    MonthRstConfig config; // 持有一个配置对象

public:
    // 构造函数，允许传入一个自定义的配置对象，也支持使用默认配置
    explicit MonthRstFormat(const MonthRstConfig& config = MonthRstConfig());

    // 使用 override 关键字实现接口
    std::string format_report(const MonthlyReportData& data) const override;
};

#endif // MONTH_RST_FORMAT_HPP