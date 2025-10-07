// reprocessing/Reprocessor.cpp
#include "Reprocessor.hpp"
#include "reprocessing/validator/result/ValidationResult.hpp" // 需要包含此文件以使用 ValidationResult
#include "common/common_utils.hpp" // 用于颜色输出
#include <iostream>
#include <fstream>
#include <sstream>

// 构造函数保持不变
Reprocessor::Reprocessor(const nlohmann::json& validator_config_json, const nlohmann::json& modifier_config_json) {
    try {
        m_validator = std::make_unique<BillValidator>(validator_config_json);
        m_modifier = std::make_unique<BillModifier>(modifier_config_json);
    } catch (const std::exception& e) {
        std::cerr << "在 Reprocessor 内部组件初始化过程中发生错误: " << e.what() << std::endl;
        throw;
    }
}


// --- MODIFIED: validate_bill 方法实现了新的流程编排逻辑 ---
bool Reprocessor::validate_bill(const std::string& bill_path) {
    std::cout << "\n================================================================\n"
              << "Validating file: " << bill_path << "\n"
              << "----------------------------------------------------------------";
              
    // 读取文件内容
    std::ifstream input_file(bill_path);
    if (!input_file.is_open()) {
        std::cerr << "错误: 无法打开输入账单文件 '" << bill_path << "'\n";
        return false;
    }
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    std::string bill_content = buffer.str();
    input_file.close();

    ValidationResult result;
    bool is_valid = true;

    // 第 1 步: 验证 TXT 基本结构
    std::cout << "\n--- Step 1: Validating TXT structure ---\n";
    if (!m_validator->validate_txt_structure(bill_content, result)) {
        is_valid = false;
    }

    result.print_report();
    if (!is_valid) {
        std::cout << RED_COLOR << "Result: Basic TXT structure validation FAILED." << RESET_COLOR << std::endl;
        std::cout << "================================================================\n";
        return false;
    } else {
        std::cout << GREEN_COLOR << "Result: Basic TXT structure validation PASSED." << RESET_COLOR << std::endl;
    }
    
    // 第 2 步: 将 TXT 转换为 JSON
    std::cout << "\n--- Step 2: Converting TXT to JSON ---\n";
    std::string json_string;
    nlohmann::json bill_json;
    try {
        json_string = m_modifier->modify(bill_content);
        bill_json = nlohmann::json::parse(json_string);
        std::cout << "Conversion successful.\n";
    } catch (const std::exception& e) {
        std::cerr << "错误: 在将 TXT 转换为 JSON 期间发生错误: " << e.what() << std::endl;
        is_valid = false;
    }
    
    if(!is_valid) {
        std::cout << RED_COLOR << "Result: Validation FAILED during conversion step." << RESET_COLOR << std::endl;
        std::cout << "================================================================\n";
        return false;
    }


    // 第 3 步: 验证 JSON 内容
    std::cout << "\n--- Step 3: Validating JSON content ---\n";
    result.clear(); // 为新的验证步骤清空之前的结果
    if (!m_validator->validate_json_content(bill_json, result)) {
        is_valid = false;
    }

    result.print_report();
    if (is_valid) {
        std::cout << GREEN_COLOR << "Result: Overall validation PASSED" << RESET_COLOR << std::endl;
    } else {
        std::cout << RED_COLOR << "Result: JSON content validation FAILED" << RESET_COLOR << std::endl;
    }

    std::cout << "================================================================\n";

    return is_valid;
}


// modify_bill 方法保持不变
bool Reprocessor::modify_bill(const std::string& input_bill_path, const std::string& output_bill_path) {
    try {
        std::ifstream input_file(input_bill_path);
        if (!input_file.is_open()) {
            std::cerr << "错误: 无法打开输入账单文件 '" << input_bill_path << "'\n";
            return false;
        }
        std::stringstream buffer;
        buffer << input_file.rdbuf();
        std::string bill_content = buffer.str();
        input_file.close();

        std::cout << "\n--- 正在修改: " << input_bill_path << " -> " << output_bill_path << " ---\n";
        std::string modified_content = m_modifier->modify(bill_content);

        std::ofstream output_file(output_bill_path);
        if (!output_file.is_open()) {
            std::cerr << "错误: 无法打开输出账单文件 '" << output_bill_path << "' 进行写入。\n";
            return false;
        }
        output_file << modified_content;
        output_file.close();
        
        std::cout << "--- 修改成功。输出已保存至 '" << output_bill_path << "' ---\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "在修改过程中发生错误: " << e.what() << std::endl;
        return false;
    }
}