// ports/output_path_builder.hpp
#ifndef PORTS_OUTPUT_PATH_BUILDER_H_
#define PORTS_OUTPUT_PATH_BUILDER_H_

#include <filesystem>

class OutputPathBuilder {
 public:
  virtual ~OutputPathBuilder() = default;
  [[nodiscard]] virtual auto build_output_path(
      const std::filesystem::path& input_file) const
      -> std::filesystem::path = 0;
};

#endif  // PORTS_OUTPUT_PATH_BUILDER_H_
