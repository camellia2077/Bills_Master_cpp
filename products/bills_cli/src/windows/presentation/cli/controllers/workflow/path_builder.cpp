// controllers/workflow/PathBuilder.cpp
#include "path_builder.hpp"

#include <iostream>

#include "common/common_utils.hpp"
#include "windows/infrastructure/file_handler/file_handler.hpp"

PathBuilder::PathBuilder(const std::string& base_output_dir)
    : m_base_output_dir(base_output_dir) {}

auto PathBuilder::build_output_path(const fs::path& input_file) const
    -> fs::path {
  fs::path modified_path = input_file;
  modified_path.replace_extension(".json");
  std::string filename_stem = modified_path.stem().string();

  fs::path final_output_path;
  if (filename_stem.length() >= 4) {
    std::string year = filename_stem.substr(0, 4);
    fs::path target_dir = m_base_output_dir / year;
    FileHandler::create_directories(target_dir);
    final_output_path = target_dir / modified_path.filename();
  } else {
    std::cerr << YELLOW_COLOR << "警告: " << RESET_COLOR << "无法从文件名 '"
              << modified_path.filename().string()
              << "' 中确定年份。文件将保存在输出根目录。\n";
    FileHandler::create_directories(m_base_output_dir);
    final_output_path = m_base_output_dir / modified_path.filename();
  }
  return final_output_path;
}

