// YearTexFormat.h
#ifndef YEAR_TEX_FORMAT_H
#define YEAR_TEX_FORMAT_H

#include "year/year_format/IYearlyReportFormatter.h"

class YearTexFormat : public IYearlyReportFormatter {
public:
    std::string format_report(const YearlyReportData& data) const override;
};

#endif // YEAR_TEX_FORMAT_H