#ifndef YEAR_TYP_FORMAT_H
#define YEAR_TYP_FORMAT_H

#include "query/plugins/year_formatters/BaseYearlyReportFormatter.hpp" 
#include "YearlyTypConfig.hpp"

class YearTypFormat : public BaseYearlyReportFormatter {
public:
    explicit YearTypFormat(const YearlyTypConfig& config = YearlyTypConfig{});

private:
    YearlyTypConfig config_;

protected:
    std::string get_no_data_message(int year) const override;
    std::string generate_header(const YearlyReportData& data) const override;
    std::string generate_summary(const YearlyReportData& data) const override;
    std::string generate_monthly_breakdown_header() const override;
    std::string generate_monthly_item(int year, int month, double total) const override;
    std::string generate_footer(const YearlyReportData& data) const override;
};

#endif // YEAR_TYP_FORMAT_H