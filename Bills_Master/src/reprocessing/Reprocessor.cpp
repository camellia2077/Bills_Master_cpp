// reprocessing/Reprocessor.cpp
// src/reprocessing/Reprocessor.cpp
#include "Reprocessor.hpp"
#include <iostream>

// --- 构造函数 ---
// --- 构造函数 ---
// **修改**: 构造函数现在接收已验证的JSON对象，不再负责文件加载或验证。
Reprocessor::Reprocessor(const nlohmann::json& validator_config_json, const nlohmann::json& modifier_config_json) {
    try {
        m_validator = std::make_unique<BillValidator>(validator_config_json);
        m_modifier = std::make_unique<BillModifier>(modifier_config_json);
    } catch (const std::exception& e) {
        // 如果在构造过程中发生任何错误，立即报告并重新抛出异常
        std::cerr << "在 Reprocessor 内部组件初始化过程中发生错误: " << e.what() << std::endl;
        throw;
    }
}

// --- 公共方法 (保持不变) ---

bool Reprocessor::validate_bill(const std::string& bill_path) {
    try {
        return m_validator->validate(bill_path);
    } catch (const std::runtime_error& e) {
        std::cerr << "在验证过程中发生严重错误: " << e.what() << std::endl;
        return false;
    }
}

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