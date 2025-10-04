// app_controller/workflow/PathBuilder.cpp
#include "PathBuilder.hpp"
#include "file_handler/FileHandler.hpp" // 包含 FileHandler
#include "common/common_utils.hpp"
#include <iostream>

PathBuilder::PathBuilder(const std::string& base_output_dir, FileHandler& file_handler)
    : m_base_output_dir(base_output_dir), m_file_handler(file_handler) {}

fs::path PathBuilder::build_output_path(const fs::path& input_file) const {
    fs::path modified_path = input_file;
    modified_path.replace_extension(".json");
    std::string filename_stem = modified_path.stem().string();

    fs::path final_output_path;
    if (filename_stem.length() >= 4) {
        std::string year = filename_stem.substr(0, 4);
        fs::path target_dir = m_base_output_dir / year;
        m_file_handler.create_directories(target_dir); // 使用 FileHandler 创建目录
        final_output_path = target_dir / modified_path.filename();
    } else {
        std::cerr << YELLOW_COLOR << "警告: " << RESET_COLOR << "无法从文件名 '" << modified_path.filename().string() << "' 中确定年份。文件将保存在输出根目录。\n";
        m_file_handler.create_directories(m_base_output_dir); // 使用 FileHandler 创建目录
        final_output_path = m_base_output_dir / modified_path.filename();
    }
    return final_output_path;
}