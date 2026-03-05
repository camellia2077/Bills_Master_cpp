// controllers/workflow/PathBuilder.hpp
#ifndef PATH_BUILDER_HPP
#define PATH_BUILDER_HPP

#include <filesystem>
#include <string>

#include "ports/output_path_builder.hpp"

namespace fs = std::filesystem;

class PathBuilder : public OutputPathBuilder {
 public:
  explicit PathBuilder(const std::string& base_output_dir);
  auto build_output_path(const fs::path& input_file) const -> fs::path override;

 private:
  fs::path m_base_output_dir;
};

#endif  // PATH_BUILDER_HPP
