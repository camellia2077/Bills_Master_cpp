// YearTypFormat.h
#ifndef YEAR_TYP_FORMAT_H
#define YEAR_TYP_FORMAT_H

#include "query/year/year_format/IYearlyReportFormatter.h"

class YearTypFormat : public IYearlyReportFormatter {
public:
    std::string format_report(const YearlyReportData& data) const override;
};

#endif // YEAR_TYP_FORMAT_H