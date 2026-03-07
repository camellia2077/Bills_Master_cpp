// windows/presentation/cli/controllers/workflow/path_builder.hpp
#ifndef WINDOWS_PRESENTATION_CLI_CONTROLLERS_WORKFLOW_PATH_BUILDER_H_
#define WINDOWS_PRESENTATION_CLI_CONTROLLERS_WORKFLOW_PATH_BUILDER_H_

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

#endif  // WINDOWS_PRESENTATION_CLI_CONTROLLERS_WORKFLOW_PATH_BUILDER_H_
