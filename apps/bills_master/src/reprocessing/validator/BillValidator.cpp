// reprocessing/validator/BillValidator.cpp
#include "BillValidator.hpp"
#include "common/common_utils.hpp"
#include <iostream>
#include <string>

// 构造函数现在接收一个json对象
BillValidator::BillValidator(const nlohmann::json& config_json)
    : config(std::make_unique<BillConfig>(config_json)),
      processor(),
      result()
{
    std::cout << "BillValidator initialized successfully, configuration loaded.\n";
}

bool BillValidator::validate(const std::string& bill_file_path) {
    std::cout << "\n================================================================\n"
              << "Validating file: " << bill_file_path << "\n"
              << "----------------------------------------------------------------";

    bool is_valid = processor.validate(bill_file_path, *config, result);

    result.print_report();

    if (is_valid) {
        std::cout << GREEN_COLOR << "Result: Validation PASSED" << RESET_COLOR << std::endl;
    } else {
        std::cout << RED_COLOR << "Result: Validation FAILED" << RESET_COLOR << std::endl;
    }

    std::cout << "================================================================\n";

    return is_valid;
}