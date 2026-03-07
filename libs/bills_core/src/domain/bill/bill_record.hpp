// domain/bill/bill_record.hpp
#ifndef DOMAIN_BILL_BILL_RECORD_H_
#define DOMAIN_BILL_BILL_RECORD_H_

#include <string>
#include <vector>

struct Transaction {
  std::string parent_category;
  std::string sub_category;
  double amount;
  std::string description;
  std::string source;
  std::string comment;
  std::string transaction_type;
};

struct ParsedBill {
  std::string date;
  std::string remark;
  int year;
  int month;
  std::vector<Transaction> transactions;
  double total_income;
  double total_expense;
  double balance;
};

#endif  // DOMAIN_BILL_BILL_RECORD_H_
