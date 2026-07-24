#ifndef PORTS_CONTRACTS_REPORTS_RANGE_RANGE_REPORT_DATA_H_
#define PORTS_CONTRACTS_REPORTS_RANGE_RANGE_REPORT_DATA_H_

#include <string>
#include <vector>

#include "ports/contracts/reports/monthly/monthly_report_data.hpp"

struct RangeReportData {
  // Canonical report query result for the reporting core.
  // Single-month and yearly reports are projected from this range model
  // instead of using separate core query result types.
  std::string period_start;
  std::string period_end;
  bool data_found = false;

  double total_income = 0.0;
  double total_expense = 0.0;
  double balance = 0.0;

  std::vector<MonthlyReportData> months;
};

#endif  // PORTS_CONTRACTS_REPORTS_RANGE_RANGE_REPORT_DATA_H_
