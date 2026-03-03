#ifndef REPORTS_STANDARD_JSON_STANDARD_REPORT_DTO_HPP
#define REPORTS_STANDARD_JSON_STANDARD_REPORT_DTO_HPP

#include <string>
#include <vector>

struct StandardTransactionItem {
  std::string parent_category;
  std::string sub_category;
  std::string transaction_type;
  std::string description;
  std::string source;
  std::string comment;
  double amount = 0.0;
};

struct StandardSubCategoryItem {
  std::string name;
  double subtotal = 0.0;
  std::vector<StandardTransactionItem> transactions;
};

struct StandardCategoryItem {
  std::string name;
  double total = 0.0;
  std::vector<StandardSubCategoryItem> sub_categories;
};

struct StandardMonthlySummaryItem {
  int month = 0;
  double income = 0.0;
  double expense = 0.0;
  double balance = 0.0;
};

struct StandardReport {
  std::string schema_version = "1.0.0";
  std::string report_type;
  std::string generated_at_utc;
  std::string source = "bills_core";

  std::string period_start;
  std::string period_end;
  std::string remark;
  bool data_found = false;

  double total_income = 0.0;
  double total_expense = 0.0;
  double balance = 0.0;

  std::vector<StandardCategoryItem> categories;
  std::vector<StandardMonthlySummaryItem> monthly_summary;
};

#endif  // REPORTS_STANDARD_JSON_STANDARD_REPORT_DTO_HPP
