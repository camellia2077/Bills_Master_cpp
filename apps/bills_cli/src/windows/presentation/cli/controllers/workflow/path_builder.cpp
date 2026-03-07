// windows/presentation/cli/controllers/workflow/path_builder.cpp
#include "path_builder.hpp"

#include <iostream>
#include <system_error>

#include "common/common_utils.hpp"

namespace terminal = common::terminal;

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
    std::error_code target_dir_error;
    fs::create_directories(target_dir, target_dir_error);
    if (target_dir_error) {
      std::cerr << terminal::kYellow << "警告: " << terminal::kReset
                << "无法创建输出目录 '" << target_dir.string()
                << "'，错误: " << target_dir_error.message() << "\n";
    }
    final_output_path = target_dir / modified_path.filename();
  } else {
    std::cerr << terminal::kYellow << "警告: " << terminal::kReset << "无法从文件名 '"
              << modified_path.filename().string()
              << "' 中确定年份。文件将保存在输出根目录。\n";
    std::error_code base_dir_error;
    fs::create_directories(m_base_output_dir, base_dir_error);
    if (base_dir_error) {
      std::cerr << terminal::kYellow << "警告: " << terminal::kReset
                << "无法创建输出目录 '" << m_base_output_dir.string()
                << "'，错误: " << base_dir_error.message() << "\n";
    }
    final_output_path = m_base_output_dir / modified_path.filename();
  }
  return final_output_path;
}

