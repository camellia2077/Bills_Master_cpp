#include "ValidationResult.hpp"
#include "common/common_utils.hpp" // For using color macros

void ValidationResult::add_error(const std::string& message) {
    errors.push_back(message);
}

void ValidationResult::add_warning(const std::string& message) {
    warnings.push_back(message);
}

bool ValidationResult::has_errors() const {
    return !errors.empty();
}

// MODIFIED print_report method: Added colors for errors and warnings
void ValidationResult::print_report() const {
    std::cout << "\n--- Validation Report ---\n";
    if (errors.empty() && warnings.empty()) {
        std::cout << "Validation passed, no errors or warnings found.\n";
        return;
    }

    if (!errors.empty()) {
        // Errors are now printed in red
        std::cerr << RED_COLOR << "Found " << errors.size() << " error(s):\n" << RESET_COLOR;
        for (const auto& err : errors) {
            std::cerr << "- " << err << std::endl;
        }
    }

    if (!warnings.empty()) {
        // Warnings are now printed in yellow
        std::cout << YELLOW_COLOR << "Found " << warnings.size() << " warning(s):\n" << RESET_COLOR;
        for (const auto& warn : warnings) {
            std::cout << "- " << warn << std::endl;
        }
    }
    std::cout << "--- End of Report ---\n\n";
}

void ValidationResult::clear() {
    errors.clear();
    warnings.clear();
}