// reprocessing/validator/BillValidator.cpp
#include "BillValidator.hpp"
#include "common/common_utils.hpp" // For using color macros
#include <iostream>
#include <string>

// The constructor remains unchanged
BillValidator::BillValidator(const std::string& config_path)
    : config(std::make_unique<BillConfig>(config_path)),
      processor(),
      result()
{
    std::cout << "BillValidator initialized successfully, configuration loaded.\n";
}

// MODIFIED validate method: This now controls all output for a single file.
bool BillValidator::validate(const std::string& bill_file_path) {
    // Start of the new formatted block for a single file
    std::cout << "\n================================================================\n"
              << "Validating file: " << bill_file_path << "\n"
              << "----------------------------------------------------------------";

    // Call the processor to perform the validation
    bool is_valid = processor.validate(bill_file_path, *config, result);

    // Print the detailed report (errors/warnings)
    result.print_report();

    // Print the final summary line with the appropriate color
    if (is_valid) {
        std::cout << GREEN_COLOR << "Result: Validation PASSED" << RESET_COLOR << std::endl;
    } else {
        std::cout << RED_COLOR << "Result: Validation FAILED" << RESET_COLOR << std::endl;
    }

    // End of the formatted block
    std::cout << "================================================================\n";

    return is_valid;
}