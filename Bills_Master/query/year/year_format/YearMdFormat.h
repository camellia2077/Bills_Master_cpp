// YearMdFormat.h
#ifndef YEAR_MD_FORMAT_H
#define YEAR_MD_FORMAT_H

#include <string>
#include "year/_year_data/YearlyReportData.h"

class YearMdFormat {
public:
    // Formats the data from the structure into a report string.
    std::string format_report(const YearlyReportData& data);
};

#endif // YEAR_MD_FORMAT_H