#include "AppController.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <string>

#include "reprocessing/Reprocessor.h"
#include "db_insert/DataProcessor.h"
#include "query/QueryDb.h"
#include "file_handler/FileHandler.h"
#include "common/version.h"
#include "common/common_utils.h"

namespace fs = std::filesystem;

AppController::AppController() {
    // 构造函数
}

void AppController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- 正在验证: " << file.string() << " ---\n";
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
    stats.print_summary("验证");
}

void AppController::handle_modification(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::string filename_stem = file.stem().string();
            fs::path modified_path;
            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path("txt_raw") / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / file.filename();
            } else {
                std::cerr << YELLOW_COLOR << "警告: " << RESET_COLOR << "无法从文件名 '" << file.filename().string() << "' 确定年份。保存在根 txt_raw 目录中。\n";
                fs::path target_dir("txt_raw");
                fs::create_directory(target_dir);
                modified_path = target_dir / file.filename();
            }
            
            std::cout << "\n--- 正在修改: " << file.string() << " -> " << modified_path.string() << " ---\n";
            if(reprocessor.modify_bill(file.string(), modified_path.string())) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("修改");
}

void AppController::handle_import(const std::string& path) {
    ProcessStats stats;
    const std::string db_path = "bills.db";
    std::cout << "正在使用数据库文件: " << db_path << "\n";

    try {
        FileHandler file_handler;
        DataProcessor data_processor;
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- 正在为数据库处理: " << file.string() << " ---\n";
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
    stats.print_summary("数据库导入");
}

void AppController::handle_full_workflow(const std::string& path) {
    ProcessStats stats;
    std::cout << "--- 自动处理工作流已启动 ---\n";
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        DataProcessor data_processor;

        std::vector<fs::path> files = file_handler.find_txt_files(path);
        if (files.empty()) {
            std::cout << "未找到要处理的文件。\n";
            return;
        }
        
        for (const auto& file_path : files) {
            std::cout << "\n========================================\n";
            std::cout << "正在处理文件: " << file_path.string() << "\n";
            std::cout << "========================================\n";
            
            std::cout << "\n[步骤 1/3] 正在验证账单文件...\n";
            if (!reprocessor.validate_bill(file_path.string())) {
                std::cerr << RED_COLOR << "验证失败" << RESET_COLOR << " 对于 " << file_path.string() << ". 跳过此文件。" << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "验证完成。" << "\n";
            
            std::string filename_stem = file_path.stem().string();
            fs::path modified_path;
            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path("txt_raw") / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / file_path.filename();
            } else {
                fs::path target_dir = fs::path("txt_raw");
                fs::create_directory(target_dir);
                modified_path = target_dir / file_path.filename();
            }

            std::cout << "\n[步骤 2/3] 正在修改账单文件...\n";
            if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                std::cerr << RED_COLOR << "修改失败" << RESET_COLOR << " 对于 " << file_path.string() << ". 跳过此文件。" << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "修改完成。修改后的文件已保存至 '" << modified_path.string() << "'。\n";
            
            std::cout << "\n[步骤 3/3] 正在解析并插入数据库...\n";
            const std::string db_path = "bills.db";
            if (data_processor.process_and_insert(modified_path.string(), db_path)) {
                std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "此文件的数据库导入完成。" << "\n";
                stats.success++;
            } else {
                std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR << " 对于此文件。" << "\n";
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << "工作流期间发生错误: " << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("完整工作流");
}


void AppController::handle_export(const std::string& type, const std::string& value, const std::string& format_str) {
    try {
        // 将格式字符串转换为 ReportFormat 枚举
        ReportFormat format;
        if (format_str == "tex") {
            format = ReportFormat::LaTeX;
        } else if (format_str == "typ") { 
            format = ReportFormat::Typst;
        } else if (format_str == "md") {
            format = ReportFormat::Markdown;
        } else if (format_str == "rst") {
            format = ReportFormat::Rst;
        } else {throw std::runtime_error("你输入的格式暂时还没有支持: " + format_str + "。请使用 'md', 'tex', 或 'typ'。");}

        QueryFacade facade("bills.db");
        if (type == "all") {
            facade.export_all_reports(format);
        } else if (type == "year") {
            if (value.empty()) {
                throw std::runtime_error("导出年度报告需要提供年份。");
            }
            facade.export_yearly_report(value, format);
        } else if (type == "month") {
            if (value.empty()) {
                throw std::runtime_error("导出月度报告需要提供月份 (YYYYMM)。");
            }
            facade.export_monthly_report(value, format);
        } else {
            throw std::runtime_error("未知的导出类型: " + type);
        }
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "导出失败: " << RESET_COLOR << e.what() << std::endl;
    }
}

void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}