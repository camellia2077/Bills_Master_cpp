#ifndef PORTS_CONTRACTS_REPORTS_MONTHLY_MONTHLY_REPORT_DATA_HPP
#define PORTS_CONTRACTS_REPORTS_MONTHLY_MONTHLY_REPORT_DATA_HPP

#include <map>
#include <string>
#include <vector>

#include "domain/bill/bill_record.hpp"

struct SubCategoryData {
  double sub_total = 0.0;
  std::vector<Transaction> transactions;
};

struct ParentCategoryData {
  double parent_total = 0.0;
  std::map<std::string, SubCategoryData> sub_categories;
};

struct MonthlyReportData {
  int year;
  int month;
  std::string remark;
  std::map<std::string, ParentCategoryData> aggregated_data;
  bool data_found = false;

  double total_income = 0.0;
  double total_expense = 0.0;
  double balance = 0.0;
};

#endif  // PORTS_CONTRACTS_REPORTS_MONTHLY_MONTHLY_REPORT_DATA_HPP
