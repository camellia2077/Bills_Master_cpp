// YearRstFormat.h
#ifndef YEAR_RST_FORMAT_H
#define YEAR_RST_FORMAT_H

#include "year/year_format/IYearlyReportFormatter.h"

class YearRstFormat : public IYearlyReportFormatter {
public:
    std::string format_report(const YearlyReportData& data) const override;
};

#endif // YEAR_RST_FORMAT_H