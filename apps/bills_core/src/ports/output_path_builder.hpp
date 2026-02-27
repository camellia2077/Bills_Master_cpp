// ports/output_path_builder.hpp
#ifndef OUTPUT_PATH_BUILDER_HPP
#define OUTPUT_PATH_BUILDER_HPP

#include <filesystem>

class OutputPathBuilder {
 public:
  virtual ~OutputPathBuilder() = default;
  [[nodiscard]] virtual auto build_output_path(
      const std::filesystem::path& input_file) const
      -> std::filesystem::path = 0;
};

#endif  // OUTPUT_PATH_BUILDER_HPP
