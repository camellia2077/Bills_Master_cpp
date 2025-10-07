// reprocessing/validator/result/ValidationResult.cpp
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


// --- DEVELOPER NOTE ---
//
// **关于为何将错误输出从 std::cerr 改为 std::cout 的说明**
//
// 按照标准实践，错误信息通常应输出到标准错误流 (std::cerr)，而常规信息输出到标准输出流 (std::cout)。
// 然而，在本项目中，我们特意将所有输出（包括错误）都定向到了 std::cout。
//
// **理由如下:**
// 我们的自动化测试脚本会捕获命令行程序的输出并生成日志文件。如果使用两个独立的流，
// 脚本将无法保证输出的严格时序，导致日志中所有常规信息在前，所有错误信息在后，
// 这使得调试（例如，定位是哪个文件验证失败及其原因）变得非常困难。
//
// 通过将所有内容输出到同一个流，我们确保了日志的 **时序一致性** 和 **可读性**。
// 错误信息会紧跟在产生它的操作之后，极大地提升了调试效率。
//
// 在这个特定的测试场景下，我们认为 **日志的易读性比严格遵守流分离的规范更重要**。
// 程序依然通过 **退出码 (Exit Code)** 来向外部脚本报告成功或失败的状态，这是更健壮的机制。
//
void ValidationResult::print_report() const {
    std::cout << "\n--- Validation Report ---\n";
    if (errors.empty() && warnings.empty()) {
        std::cout << "Validation passed, no errors or warnings found.\n";
        return;
    }

    if (!errors.empty()) {
        std::cout << RED_COLOR << "Found " << errors.size() << " error(s):\n" << RESET_COLOR;
        for (const auto& err : errors) {
            std::cout << "- " << err << std::endl;
        }
    }

    if (!warnings.empty()) {
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