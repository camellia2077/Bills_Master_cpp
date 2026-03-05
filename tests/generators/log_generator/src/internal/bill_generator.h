#ifndef BILL_GENERATOR_H
#define BILL_GENERATOR_H

#include <random>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class BillGenerator {
 public:
  BillGenerator(nlohmann::json categories, double comment_probability,
                std::vector<std::string> comments);
  auto generate_bill_content(int year, int month) const -> std::string;

 private:
  auto random_double(double min, double max) const -> double;
  auto random_int(int min, int max) const -> int;

  nlohmann::json categories_;
  double comment_probability_ = 0.0;
  std::vector<std::string> comments_;
  mutable std::mt19937 random_engine_;
};

#endif  // BILL_GENERATOR_H
