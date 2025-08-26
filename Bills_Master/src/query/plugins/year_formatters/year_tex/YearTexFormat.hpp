#ifndef YEAR_TEX_FORMAT_H
#define YEAR_TEX_FORMAT_H

#include "query/plugins/year_formatters/BaseYearlyReportFormatter.hpp" 
#include "YearlyTexConfig.hpp"

class YearTexFormat : public BaseYearlyReportFormatter { 
public:
    explicit YearTexFormat(const YearlyTexConfig& config = YearlyTexConfig{});

private:
    std::string escape_latex(const std::string& input) const;
    YearlyTexConfig m_config;

protected:
    std::string get_no_data_message(int year) const override;
    std::string generate_header(const YearlyReportData& data) const override;
    std::string generate_summary(const YearlyReportData& data) const override;
    std::string generate_monthly_breakdown_header() const override;
    std::string generate_monthly_item(int year, int month, double total) const override;
    std::string generate_footer(const YearlyReportData& data) const override;
};

#endif // YEAR_TEX_FORMAT_H