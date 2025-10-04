// reports/plugins/month_formatters/IMonthReportFormatter.hpp
#ifndef I_MONTH_REPORT_FORMATTER_HPP
#define I_MONTH_REPORT_FORMATTER_HPP

#include <string>

#include "reports/components/monthly_report/ReportData.hpp" 

/**
 * @class IMonthReportFormatter
 * @brief 报表格式化器的通用接口（抽象基类）。
 *
 * 定义了一个所有具体格式化器都必须实现的通用接口。
 * 这允许客户端代码以统一的方式处理不同的报表格式。
 */
class IMonthReportFormatter {
public:
    virtual ~IMonthReportFormatter() = default; // 虚析构函数，保证派生类对象能被正确销毁

    /**
     * @brief 格式化月度报告数据。
     * @param data 包含所有报告数据的 MonthlyReportData 结构体。
     * @return 一个表示完整报告的格式化字符串。
     */
    virtual std::string format_report(const MonthlyReportData& data) const = 0; // 纯虚函数
};

#endif // I_MONTH_REPORT_FORMATTER_HPP