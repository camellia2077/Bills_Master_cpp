
#include "ValidationResult.h"

void ValidationResult::add_error(const std::string& message) {
    errors.push_back(message);
}

void ValidationResult::add_warning(const std::string& message) {
    warnings.push_back(message);
}

bool ValidationResult::has_errors() const {
    return !errors.empty();
}

void ValidationResult::print_report() const {
    std::cout << "\n--- Validation Report ---\n";
    if (errors.empty() && warnings.empty()) {
        std::cout << "Validation passed, no errors or warnings found.\n";
        return;
    }

    if (!errors.empty()) {
        std::cerr << "Found " << errors.size() << " errors:\n";
        for (const auto& err : errors) {
            std::cerr << "- " << err << std::endl;
        }
    }

    if (!warnings.empty()) {
        std::cerr << "Found " << errors.size() << " warnings:\n";
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
