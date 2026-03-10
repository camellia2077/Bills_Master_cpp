// billing/conversion/validator/result/validation_result.cpp
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

auto ValidationResult::error_messages() const
    -> const std::vector<std::string>& {
  return errors;
}

auto ValidationResult::warning_messages() const
    -> const std::vector<std::string>& {
  return warnings;
}

void ValidationResult::print_report() const {}

void ValidationResult::clear() {
  errors.clear();
  warnings.clear();
}
