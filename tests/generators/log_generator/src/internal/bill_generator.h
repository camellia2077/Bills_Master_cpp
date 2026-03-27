#ifndef BILL_GENERATOR_H
#define BILL_GENERATOR_H

#include <random>
#include <string>
#include <vector>

#include "config_io.h"

class BillGenerator {
 public:
  BillGenerator(std::vector<GeneratorCategoryConfig> categories,
                double comment_probability,
                std::vector<std::string> comments,
                std::vector<std::string> remark_summary_lines,
                std::vector<std::string> remark_followup_lines);
  auto generate_bill_content(int year, int month) const -> std::string;

 private:
  auto build_remark_lines(int year, int month) const -> std::vector<std::string>;
  auto random_double(double min, double max) const -> double;
  auto random_int(int min, int max) const -> int;

  std::vector<GeneratorCategoryConfig> categories_;
  double comment_probability_ = 0.0;
  std::vector<std::string> comments_;
  std::vector<std::string> remark_summary_lines_;
  std::vector<std::string> remark_followup_lines_;
  mutable std::mt19937 random_engine_;
};

#endif  // BILL_GENERATOR_H
