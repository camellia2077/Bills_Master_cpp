// conversion/Reprocessor.cpp
#include "Reprocessor.hpp"
#include "conversion/validator/result/ValidationResult.hpp" // 需要包含此文件以使用 ValidationResult
#include "common/common_utils.hpp" // 用于颜色输出
#include <iostream>

// 构造函数保持不变
Reprocessor::Reprocessor(const nlohmann::json& validator_config_json, const nlohmann::json& modifier_config_json) {
    try {
        m_validator = std::make_unique<BillValidator>(validator_config_json);
        m_converter = std::make_unique<BillConverter>(modifier_config_json);
    } catch (const std::exception& e) {
        std::cerr << "在 Reprocessor 内部组件初始化过程中发生错误: " << e.what() << std::endl;
        throw;
    }
}


// --- MODIFIED: validate_content 方法实现了新的流程编排逻辑 ---
auto Reprocessor::validate_content(const std::string& bill_content,
                                   const std::string& source_name) -> bool {
  ParsedBill bill_data{};
  return validate_and_convert_content(bill_content, source_name, bill_data);
}

auto Reprocessor::validate_and_convert_content(const std::string& bill_content,
                                               const std::string& source_name,
                                               ParsedBill& bill_data) -> bool {
  std::cout
      << "\n================================================================\n"
      << "Validating file: " << source_name << "\n"
      << "----------------------------------------------------------------";

  ValidationResult result;
  bool is_valid = true;

  // 第 1 步: 验证 TXT 基本结构
  std::cout << "\n--- Step 1: Validating TXT structure ---\n";
  if (!m_validator->validate_txt_structure(bill_content, result)) {
    is_valid = false;
  }

  result.print_report();
  if (!is_valid) {
    std::cout << RED_COLOR << "Result: Basic TXT structure validation FAILED."
              << RESET_COLOR << std::endl;
    std::cout
        << "================================================================\n";
    return false;
  }
  std::cout << GREEN_COLOR << "Result: Basic TXT structure validation PASSED."
            << RESET_COLOR << std::endl;

  // 第 2 步: 将 TXT 转换为领域模型
  std::cout << "\n--- Step 2: Converting TXT to Domain Model ---\n";
  try {
    bill_data = m_converter->convert(bill_content);
    std::cout << "Conversion successful.\n";
  } catch (const std::exception& e) {
    std::cerr << "错误: 在将 TXT 转换为领域模型期间发生错误: " << e.what()
              << std::endl;
    is_valid = false;
  }

  if (!is_valid) {
    std::cout << RED_COLOR
              << "Result: Validation FAILED during conversion step."
              << RESET_COLOR << std::endl;
    std::cout
        << "================================================================\n";
    return false;
  }

  // 第 3 步: 验证领域模型内容
  std::cout << "\n--- Step 3: Validating domain content ---\n";
  result.clear();  // 为新的验证步骤清空之前的结果
  if (!m_validator->validate_bill_content(bill_data, result)) {
    is_valid = false;
  }

  result.print_report();
  if (is_valid) {
    std::cout << GREEN_COLOR << "Result: Overall validation PASSED"
              << RESET_COLOR << std::endl;
  } else {
    std::cout << RED_COLOR << "Result: Domain content validation FAILED"
              << RESET_COLOR << std::endl;
  }

  std::cout
      << "================================================================\n";

  return is_valid;
}

auto Reprocessor::convert_content(const std::string& bill_content,
                                  ParsedBill& bill_data) -> bool {
  try {
    bill_data = m_converter->convert(bill_content);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "在转换过程中发生错误: " << e.what() << std::endl;
    return false;
  }
}
