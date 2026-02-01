// conversion/validator/BillValidator.cpp
#include "BillValidator.hpp"
#include "common/common_utils.hpp"
#include <iostream>
#include <string>
#include <sstream>

// --- NEW: TxtStructureVerifier implementation ---
auto TxtStructureVerifier::verify(const std::string& bill_content,
                                  ValidationResult& result) -> bool {
  std::istringstream stream(bill_content);
  std::string line;
  int line_num = 0;

  // Validate date
  if (std::getline(stream, line)) {
    line_num++;
    if (!std::regex_match(line, std::regex(R"(^date:\d{6}$)"))) {
      result.add_error("Error (Line " + std::to_string(line_num) +
                       "): The first line must be 'date:YYYYMM'. Found: '" +
                       line + "'");
      return false;
    }
  } else {
    result.add_error(
        "Error: Bill content is empty or has less than two lines.");
    return false;
  }

  // Validate remark
  if (std::getline(stream, line)) {
    line_num++;
    if (!std::regex_match(line, std::regex(R"(^remark:.*)"))) {
      result.add_error(
          "Error (Line " + std::to_string(line_num) +
          "): The second line must start with 'remark:'. Found: '" + line +
          "'");
      return false;
    }
  } else {
    result.add_error("Error: Bill content has less than two lines.");
    return false;
  }

  return true;
}

// --- NEW: BillContentValidator implementation ---
auto BillContentValidator::verify(const ParsedBill& bill_data,
                                  const BillConfig& config,
                                  ValidationResult& result) -> bool {
  for (const auto& transaction : bill_data.transactions) {
    if (!config.is_parent_title(transaction.parent_category)) {
      result.add_error(
          "Content Error: Parent item '" + transaction.parent_category +
          "' is not a valid parent title defined in the configuration.");
    }

    if (!config.is_valid_sub_title(transaction.parent_category,
                                   transaction.sub_category)) {
      result.add_error("Content Error: Sub-category '" +
                       transaction.sub_category +
                       "' is not a valid sub-item for parent '" +
                       transaction.parent_category + "'.");
    }
  }

  return !result.has_errors();
}

// --- BillValidator Implementation ---
BillValidator::BillValidator(const nlohmann::json& config_json)
    : m_config(std::make_unique<BillConfig>(config_json))
{
    std::cout << "BillValidator initialized successfully, configuration loaded.\n";
}

auto BillValidator::validate_txt_structure(const std::string& bill_content,
                                           ValidationResult& result) -> bool {
  return m_txt_verifier.verify(bill_content, result);
}

auto BillValidator::validate_bill_content(const ParsedBill& bill_data,
                                          ValidationResult& result) -> bool {
  return m_bill_verifier.verify(bill_data, *m_config, result);
}
