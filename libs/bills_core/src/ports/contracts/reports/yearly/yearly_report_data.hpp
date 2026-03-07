// ports/contracts/reports/yearly/yearly_report_data.hpp
#ifndef PORTS_CONTRACTS_REPORTS_YEARLY_YEARLY_REPORT_DATA_H_
#define PORTS_CONTRACTS_REPORTS_YEARLY_YEARLY_REPORT_DATA_H_

#include <map>

struct MonthlySummary {
  double income = 0.0;
  double expense = 0.0;
};

struct YearlyReportData {
  int year;
  bool data_found = false;

  double total_income = 0.0;
  double total_expense = 0.0;
  double balance = 0.0;

  std::map<int, MonthlySummary> monthly_summary;
};

#endif  // PORTS_CONTRACTS_REPORTS_YEARLY_YEARLY_REPORT_DATA_H_
