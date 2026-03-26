// io/adapters/io/year_partition_output_path_builder.hpp
#ifndef BILLS_IO_ADAPTERS_IO_YEAR_PARTITION_OUTPUT_PATH_BUILDER_H_
#define BILLS_IO_ADAPTERS_IO_YEAR_PARTITION_OUTPUT_PATH_BUILDER_H_

#include <filesystem>
#include <string>

class YearPartitionOutputPathBuilder {
 public:
  explicit YearPartitionOutputPathBuilder(std::string base_output_dir);

  [[nodiscard]] auto build_output_path(
      const std::filesystem::path& input_file) const -> std::filesystem::path;

 private:
  std::filesystem::path base_output_dir_;
};

#endif  // BILLS_IO_ADAPTERS_IO_YEAR_PARTITION_OUTPUT_PATH_BUILDER_H_
