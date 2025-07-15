// YearlyReportFormatter.h
#ifndef YEARLY_REPORT_FORMATTER_H
#define YEARLY_REPORT_FORMATTER_H

#include <string>
#include "year/_year_data/YearlyReportData.h"

class YearlyReportFormatter {
public:
    // Formats the data from the structure into a report string.
    std::string format_report(const YearlyReportData& data);
};

#endif // YEARLY_REPORT_FORMATTER_H