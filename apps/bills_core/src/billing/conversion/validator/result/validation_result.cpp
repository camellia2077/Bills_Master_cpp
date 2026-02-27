// conversion/validator/result/validation_result.cpp
#include "validation_result.hpp"

void ValidationResult::add_error(const std::string& message) {
  errors.push_back(message);
}

void ValidationResult::add_warning(const std::string& message) {
  warnings.push_back(message);
}

auto ValidationResult::has_errors() const -> bool {
  return !errors.empty();
}

void ValidationResult::print_report() const {}

void ValidationResult::clear() {
  errors.clear();
  warnings.clear();
}
