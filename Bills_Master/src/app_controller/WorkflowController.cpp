// src/app_controller/WorkflowController.cpp

#include "WorkflowController.hpp"
#include "reprocessing/Reprocessor.hpp"
#include "db_insert/DataProcessor.hpp"
#include "file_handler/FileHandler.hpp"
#include "ProcessStats.hpp"
#include "common/common_utils.hpp"
#include "config_validator/ConfigValidator.hpp" // 引入配置验证器

#include <iostream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <fstream> // 用于文件读取

namespace fs = std::filesystem;

// 构造函数现在负责加载、解析和验证配置文件
WorkflowController::WorkflowController(const std::string& config_path, const std::string& modified_output_dir)
    : m_modified_output_dir(modified_output_dir)
{
    try {
        // --- 1. 加载并验证 Validator_Config.json ---
        // [FIX] Explicitly convert fs::path to std::string using .string()
        const std::string validator_config_path = (fs::path(config_path) / "Validator_Config.json").string();
        std::ifstream validator_file(validator_config_path);
        if (!validator_file.is_open()) {
            throw std::runtime_error("错误: 无法打开验证器配置文件 '" + validator_config_path + "'");
        }
        validator_file >> m_validator_config; // 解析JSON

        std::string error_msg;
        if (!ConfigValidator::validate_validator_config(m_validator_config, error_msg)) {
            throw std::runtime_error("Validator_Config.json 无效: " + error_msg);
        }

        // --- 2. 加载并验证 Modifier_Config.json ---
        // [FIX] Explicitly convert fs::path to std::string using .string()
        const std::string modifier_config_path = (fs::path(config_path) / "Modifier_Config.json").string();
        std::ifstream modifier_file(modifier_config_path);
        if (!modifier_file.is_open()) {
            throw std::runtime_error("错误: 无法打开修改器配置文件 '" + modifier_config_path + "'");
        }
        modifier_file >> m_modifier_config; // 解析JSON

        if (!ConfigValidator::validate_modifier_config(m_modifier_config, error_msg)) {
            throw std::runtime_error("Modifier_Config.json 无效: " + error_msg);
        }
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON配置文件解析失败: " + std::string(e.what()));
    }
}

bool WorkflowController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        // 使用已加载和验证的配置来创建 Reprocessor
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".txt");
        for (const auto& file : files) {
            if (reprocessor.validate_bill(file.string())) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Validation");
    return stats.failure == 0;
}

bool WorkflowController::handle_modification(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        // 使用已加载和验证的配置来创建 Reprocessor
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".txt");
        for (const auto& file : files) {
            fs::path modified_path = file;
            modified_path.replace_extension(".json");

            std::string filename_stem = modified_path.stem().string();
            fs::path final_output_path;

            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path(m_modified_output_dir) / year;
                fs::create_directories(target_dir);
                final_output_path = target_dir / modified_path.filename();
            } else {
                std::cerr << YELLOW_COLOR << "警告: " << RESET_COLOR << "无法从文件名 '" << modified_path.filename().string() << "' 中确定年份。文件将保存在输出根目录。\n";
                fs::path target_dir(m_modified_output_dir);
                fs::create_directory(target_dir);
                final_output_path = target_dir / modified_path.filename();
            }
            
            std::cout << "\n--- 正在修改: " << file.string() << " -> " << final_output_path.string() << " ---\n";
            if(reprocessor.modify_bill(file.string(), final_output_path.string())) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Modification");
    return stats.failure == 0;
}


bool WorkflowController::handle_import(const std::string& path, const std::string& db_path) {
    ProcessStats stats;
    std::cout << "正在使用数据库文件: " << db_path << "\n";

    try {
        FileHandler file_handler;
        DataProcessor data_processor;
        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".json");
        for (const auto& file : files) {
            std::cout << "\n--- 正在处理并导入数据库: " << file.string() << " ---\n";
            if (data_processor.process_and_insert(file.string(), db_path)) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Database Import");
    return stats.failure == 0;
}

bool WorkflowController::handle_full_workflow(const std::string& path, const std::string& db_path) {
    ProcessStats stats;
    std::cout << "--- 自动化处理工作流已启动 ---\n";
    try {
        FileHandler file_handler;
        // 使用已加载和验证的配置来创建 Reprocessor
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        DataProcessor data_processor;

        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".txt");
        if (files.empty()) {
            std::cout << "未找到需要处理的 .txt 文件。\n";
            stats.print_summary("Full Workflow");
            return true;
        }
        
        for (const auto& file_path : files) {
            std::cout << "\n========================================\n";
            std::cout << "正在处理文件: " << file_path.string() << "\n";
            std::cout << "========================================\n";
            
            std::cout << "\n[步骤 1/3] 正在验证账单文件...\n";
            if (!reprocessor.validate_bill(file_path.string())) {
                std::cerr << RED_COLOR << "验证失败" << RESET_COLOR << " 文件: " << file_path.string() << "。已跳过此文件。" << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "验证完成。" << "\n";
            
            fs::path temp_path = file_path;
            temp_path.replace_extension(".json");
            
            std::string filename_stem = temp_path.stem().string();
            fs::path modified_path;

            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path(m_modified_output_dir) / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / temp_path.filename();
            } else {
                fs::path target_dir(m_modified_output_dir);
                fs::create_directory(target_dir);
                modified_path = target_dir / temp_path.filename();
            }

            std::cout << "\n[步骤 2/3] 正在修改账单文件...\n";
            if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                std::cerr << RED_COLOR << "修改失败" << RESET_COLOR << " 文件: " << file_path.string() << "。已跳过此文件。" << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "修改完成。修改后的文件已保存至 '" << modified_path.string() << "'。\n";
            
            std::cout << "\n[步骤 3/3] 正在解析并插入数据库...\n";
            if (data_processor.process_and_insert(modified_path.string(), db_path)) {
                std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "此文件的数据已成功导入数据库。" << "\n";
                stats.success++;
            } else {
                std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR << " 文件: " << modified_path.string() << "。" << "\n";
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << "工作流执行期间发生错误: " << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Full Workflow");
    return stats.failure == 0;
}