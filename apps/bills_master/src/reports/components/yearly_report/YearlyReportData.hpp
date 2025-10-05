// reports/components/yearly_report/YearlyReportData.hpp
#ifndef YEARLY_REPORT_DATA_HPP
#define YEARLY_REPORT_DATA_HPP

#include <map>

// A structure to hold all data required for a yearly report.
struct YearlyReportData {
    int year;
    double grand_total = 0.0;
    std::map<int, double> monthly_totals;
    bool data_found = false;
};

#endif // YEARLY_REPORT_DATA_HPP