// query/plugins/year_formatters/BaseYearlyReportFormatter.hpp
#ifndef BASE_YEARLY_REPORT_FORMATTER_HPP
#define BASE_YEARLY_REPORT_FORMATTER_HPP

#include "IYearlyReportFormatter.hpp"

#include <string>
#include <sstream>
#include <iomanip>

class BaseYearlyReportFormatter : public IYearlyReportFormatter {
public:
    // 包含共享逻辑的主入口点，声明为 final 以防止子类重写。
    std::string format_report(const YearlyReportData& data) const final;

    virtual ~BaseYearlyReportFormatter() = default;

protected:
    // 新的纯虚函数，由具体的格式化器来实现。
    virtual std::string get_no_data_message(int year) const = 0;
    virtual std::string generate_header(const YearlyReportData& data) const = 0;
    virtual std::string generate_summary(const YearlyReportData& data) const = 0;
    virtual std::string generate_monthly_breakdown_header() const = 0;
    virtual std::string generate_monthly_item(int year, int month, double total) const = 0;
    virtual std::string generate_footer(const YearlyReportData& data) const = 0;
};

#endif // BASE_YEARLY_REPORT_FORMATTER_HPP