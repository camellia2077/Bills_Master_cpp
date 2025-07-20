// YearMdFormat.h
#ifndef YEAR_MD_FORMAT_H
#define YEAR_MD_FORMAT_H

#include "year/year_format/IYearlyReportFormatter.h" // Include the interface

class YearMdFormat : public IYearlyReportFormatter { // Inherit from the interface
public:
    // Use override to ensure it matches the base class function
    std::string format_report(const YearlyReportData& data) const override;
};

#endif // YEAR_MD_FORMAT_H