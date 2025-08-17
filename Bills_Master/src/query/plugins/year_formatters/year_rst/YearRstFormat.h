// YearRstFormat.h
#ifndef YEAR_RST_FORMAT_H
#define YEAR_RST_FORMAT_H

#include "query/plugins/year_formatters/IYearlyReportFormatter.h"

class YearRstFormat : public IYearlyReportFormatter {
public:
    std::string format_report(const YearlyReportData& data) const override;
};

#endif // YEAR_RST_FORMAT_H