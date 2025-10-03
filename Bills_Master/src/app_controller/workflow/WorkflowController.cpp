// app_controller/WorkflowController.cpp
#include "WorkflowController.hpp"
#include "app_controller/ConfigLoader.hpp"
#include "reprocessing/Reprocessor.hpp"
#include "db_insert/DataProcessor.hpp"
#include "common/ProcessStats.hpp"
#include "common/common_utils.hpp"

#include <iostream>
#include <vector>
#include <stdexcept>

namespace fs = std::filesystem;

// 构造函数现在创建并持有 FileHandler，并将其传递给依赖项
WorkflowController::WorkflowController(const std::string& config_path, const std::string& modified_output_dir)
    : m_path_builder(modified_output_dir, m_file_handler) // 将 m_file_handler 注入 PathBuilder
{
    try {
        // 将 m_file_handler 注入 ConfigLoader
        m_validator_config = ConfigLoader::load_and_validate_validator_config(config_path, m_file_handler);
        m_modifier_config = ConfigLoader::load_and_validate_modifier_config(config_path, m_file_handler);
    } catch (const std::runtime_error& e) {
        throw;
    }
}

bool WorkflowController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        // 使用成员 m_file_handler
        std::vector<fs::path> files = m_file_handler.find_files_by_extension(path, ".txt");
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
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        // 使用成员 m_file_handler
        std::vector<fs::path> files = m_file_handler.find_files_by_extension(path, ".txt");
        for (const auto& file : files) {
            fs::path final_output_path = m_path_builder.build_output_path(file);
            
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
        DataProcessor data_processor;
        // 使用成员 m_file_handler
        std::vector<fs::path> files = m_file_handler.find_files_by_extension(path, ".json");
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
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        DataProcessor data_processor;

        std::vector<fs::path> files = m_file_handler.find_files_by_extension(path, ".txt");
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
            
            fs::path modified_path = m_path_builder.build_output_path(file_path);

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