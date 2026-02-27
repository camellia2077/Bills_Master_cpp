// controllers/workflow/PathBuilder.hpp
#ifndef PATH_BUILDER_HPP
#define PATH_BUILDER_HPP

#include <filesystem>
#include <string>

#include "ports/output_path_builder.hpp"

namespace fs = std::filesystem;

// Forward declaration
class FileHandler;

class PathBuilder : public OutputPathBuilder {
 public:
  PathBuilder(const std::string& base_output_dir, FileHandler& file_handler);
  auto build_output_path(const fs::path& input_file) const -> fs::path override;

 private:
  fs::path m_base_output_dir;
  FileHandler& m_file_handler;  // 改为引用成员
};

#endif  // PATH_BUILDER_HPP
