// app_controller/workflow/WorkflowController.cpp
#include "WorkflowController.hpp"
#include "app_controller/ConfigLoader.hpp"
#include "conversion/Reprocessor.hpp"
#include "adapters/db/SqliteBillRepository.hpp"
#include "adapters/io/FileBillContentReader.hpp"
#include "adapters/io/FileBillFileEnumerator.hpp"
#include "adapters/serialization/JsonBillSerializer.hpp"
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
        m_content_reader = std::make_unique<FileBillContentReader>();
        m_file_enumerator = std::make_unique<FileBillFileEnumerator>(m_file_handler);
        m_serializer = std::make_unique<JsonBillSerializer>();
    } catch (const std::runtime_error& e) {
        throw;
    }
}

bool WorkflowController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        std::vector<fs::path> files = m_file_enumerator->ListFilesByExtension(path, ".txt");
        for (const auto& file : files) {
            try {
                std::string bill_content = m_content_reader->Read(file.string());
                if (reprocessor.validate_content(bill_content, file.string())) {
                    stats.success++;
                } else {
                    stats.failure++;
                }
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
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
    return handle_convert(path);
}

bool WorkflowController::handle_convert(const std::string& path) {
    ProcessStats stats;
    try {
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        std::vector<fs::path> files = m_file_enumerator->ListFilesByExtension(path, ".txt");
        for (const auto& file : files) {
            fs::path final_output_path = m_path_builder.build_output_path(file);
            
            std::cout << "\n--- 正在转换: " << file.string() << " -> " << final_output_path.string() << " ---\n";
            try {
                std::string bill_content = m_content_reader->Read(file.string());
                ParsedBill bill_data{};
                if (!reprocessor.convert_content(bill_content, bill_data)) {
                    stats.failure++;
                    continue;
                }
                m_serializer->WriteJson(bill_data, final_output_path.string());
                std::cout << "--- 转换成功。输出已保存至 '" << final_output_path.string() << "' ---\n";
                stats.success++;
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Conversion");
    return stats.failure == 0;
}

bool WorkflowController::handle_ingest(const std::string& path,
                                       const std::string& db_path,
                                       bool write_json) {
    ProcessStats stats;
    std::cout << "--- Ingest workflow started ---\n";
    try {
        Reprocessor reprocessor(m_validator_config, m_modifier_config);
        SqliteBillRepository repository(db_path);
        std::vector<fs::path> files = m_file_enumerator->ListFilesByExtension(path, ".txt");
        if (files.empty()) {
            std::cout << "未找到需要处理的 .txt 文件。\n";
            stats.print_summary("Ingest");
            return true;
        }

        for (const auto& file : files) {
            std::cout << "\n========================================\n";
            std::cout << "正在处理文件: " << file.string() << "\n";
            std::cout << "========================================\n";

            ParsedBill bill_data{};
            try {
                std::string bill_content = m_content_reader->Read(file.string());
                if (!reprocessor.validate_and_convert_content(bill_content, file.string(), bill_data)) {
                    std::cerr << RED_COLOR << "验证或转换失败"
                              << RESET_COLOR << " 文件: " << file.string() << "。已跳过此文件。\n";
                    stats.failure++;
                    continue;
                }
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
                stats.failure++;
                continue;
            }

            if (write_json) {
                fs::path output_path = m_path_builder.build_output_path(file);
                try {
                    m_serializer->WriteJson(bill_data, output_path.string());
                    std::cout << GREEN_COLOR << "成功: " << RESET_COLOR
                              << "JSON 已保存至 '" << output_path.string() << "'.\n";
                } catch (const std::exception& e) {
                    std::cerr << RED_COLOR << "JSON 写入失败"
                              << RESET_COLOR << " 文件: " << file.string()
                              << "，原因: " << e.what() << "\n";
                    stats.failure++;
                    continue;
                }
            }

            try {
                repository.InsertBill(bill_data);
                std::cout << GREEN_COLOR << "成功: " << RESET_COLOR
                          << "此文件的数据已成功导入数据库。\n";
                stats.success++;
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "数据库导入失败"
                          << RESET_COLOR << " 文件: " << file.string()
                          << "，原因: " << e.what() << "\n";
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR
                  << "工作流执行期间发生错误: " << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Ingest");
    return stats.failure == 0;
}

bool WorkflowController::handle_import(const std::string& path, const std::string& db_path) {
    ProcessStats stats;
    std::cout << "正在使用数据库文件: " << db_path << "\n";
    try {
        SqliteBillRepository repository(db_path);
        std::vector<fs::path> files = m_file_enumerator->ListFilesByExtension(path, ".json");
        for (const auto& file : files) {
            std::cout << "\n--- 正在处理并导入数据库: " << file.string() << " ---\n";
            try {
                ParsedBill import_bill = m_serializer->ReadJson(file.string());
                repository.InsertBill(import_bill);
                stats.success++;
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR << " 文件: " << file.string()
                          << "，原因: " << e.what() << "\n";
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
        SqliteBillRepository repository(db_path);

        std::vector<fs::path> files = m_file_enumerator->ListFilesByExtension(path, ".txt");
        if (files.empty()) {
            std::cout << "未找到需要处理的 .txt 文件。\n";
            stats.print_summary("Full Workflow");
            return true;
        }
        
        for (const auto& file_path : files) {
            std::cout << "\n========================================\n";
            std::cout << "正在处理文件: " << file_path.string() << "\n";
            std::cout << "========================================\n";
            std::string bill_content;
            try {
                bill_content = m_content_reader->Read(file_path.string());
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
                stats.failure++;
                continue;
            }
            
            std::cout << "\n[步骤 1/3] 正在验证账单文件...\n";
            if (!reprocessor.validate_content(bill_content, file_path.string())) {
                std::cerr << RED_COLOR << "验证失败" << RESET_COLOR << " 文件: " << file_path.string() << "。已跳过此文件。" << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "验证完成。" << "\n";
            
            fs::path modified_path = m_path_builder.build_output_path(file_path);

            std::cout << "\n[步骤 2/3] 正在转换账单文件...\n";
            ParsedBill bill_data{};
            if (!reprocessor.convert_content(bill_content, bill_data)) {
                std::cerr << RED_COLOR << "转换失败" << RESET_COLOR << " 文件: " << file_path.string() << "。已跳过此文件。" << "\n";
                stats.failure++;
                continue;
            }
            try {
                m_serializer->WriteJson(bill_data, modified_path.string());
                std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "转换完成。转换后的文件已保存至 '" << modified_path.string() << "'。\n";
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "JSON 写入失败" << RESET_COLOR << " 文件: " << file_path.string()
                          << "，原因: " << e.what() << "\n";
                stats.failure++;
                continue;
            }
            
            std::cout << "\n[步骤 3/3] 正在解析并插入数据库...\n";
            try {
                ParsedBill import_bill = m_serializer->ReadJson(modified_path.string());
                repository.InsertBill(import_bill);
                std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "此文件的数据已成功导入数据库。" << "\n";
                stats.success++;
            } catch (const std::exception& e) {
                std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR << " 文件: " << modified_path.string()
                          << "，原因: " << e.what() << "\n";
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
