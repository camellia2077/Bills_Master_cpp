#include "Reprocessor.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "nlohmann/json.hpp" // 为修改器加载JSON所需

// --- 构造函数 ---
// **修改**: 构造函数现在负责加载所有配置并创建验证器和修改器
Reprocessor::Reprocessor(const std::string& config_dir_path) {
    try {
        // 1. 构建验证器
        const std::string validator_config_path = config_dir_path + "/Validator_Config.json";
        m_validator = std::make_unique<BillValidator>(validator_config_path);

        // 2. 构建修改器
        const std::string modifier_config_path = config_dir_path + "/Modifier_Config.json";
        std::ifstream config_file(modifier_config_path);
        if (!config_file.is_open()) {
            throw std::runtime_error("错误: 无法打开修改器配置文件 '" + modifier_config_path + "'");
        }
        
        nlohmann::json modifier_json;
        config_file >> modifier_json;
        m_modifier = std::make_unique<BillModifier>(modifier_json);

    } catch (const std::exception& e) {
        // 如果在构造过程中发生任何错误，立即报告并重新抛出异常
        std::cerr << "在 Reprocessor 初始化过程中发生严重错误: " << e.what() << std::endl;
        throw;
    }
}

// --- 公共方法 ---

bool Reprocessor::validate_bill(const std::string& bill_path) {
    // **修改**: 直接使用成员变量 m_validator，不再每次都创建新实例
    try {
        // 简化日志: BillValidator::validate 方法本身会打印详细的验证报告
        // 所以这里不再需要额外的 "Starting/Finished" 日志
        return m_validator->validate(bill_path);
    } catch (const std::runtime_error& e) {
        std::cerr << "在验证过程中发生严重错误: " << e.what() << std::endl;
        return false;
    }
}

bool Reprocessor::modify_bill(const std::string& input_bill_path, const std::string& output_bill_path) {
    // **修改**: 直接使用成员变量 m_modifier
    try {
        // 1. 读取输入文件
        std::ifstream input_file(input_bill_path);
        if (!input_file.is_open()) {
            std::cerr << "错误: 无法打开输入账单文件 '" << input_bill_path << "'\n";
            return false;
        }
        std::stringstream buffer;
        buffer << input_file.rdbuf();
        std::string bill_content = buffer.str();
        input_file.close();

        // 2. 使用已创建的修改器来修改内容
        std::cout << "\n--- 正在修改: " << input_bill_path << " -> " << output_bill_path << " ---\n";
        std::string modified_content = m_modifier->modify(bill_content);

        // 3. 将修改后的内容写入输出文件
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