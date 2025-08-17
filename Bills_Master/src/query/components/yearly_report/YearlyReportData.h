// YearlyReportData.h
#ifndef YEARLY_REPORT_DATA_H
#define YEARLY_REPORT_DATA_H

#include <map>

// A structure to hold all data required for a yearly report.
struct YearlyReportData {
    int year;
    double grand_total = 0.0;
    std::map<int, double> monthly_totals;
    bool data_found = false;
};

#endif // YEARLY_REPORT_DATA_H