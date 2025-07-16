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
    std::cout << "\n--- 验证报告 ---\n";
    if (errors.empty() && warnings.empty()) {
        std::cout << "验证通过，未发现错误或警告。\n";
        return;
    }

    if (!errors.empty()) {
        std::cerr << "发现 " << errors.size() << " 个错误:\n";
        for (const auto& err : errors) {
            std::cerr << "- " << err << std::endl;
        }
    }

    if (!warnings.empty()) {
        std::cout << "发现 " << warnings.size() << " 个警告:\n";
        for (const auto& warn : warnings) {
            std::cout << "- " << warn << std::endl;
        }
    }
    std::cout << "--- 报告结束 ---\n";
}

void ValidationResult::clear() {
    errors.clear();
    warnings.clear();
}
