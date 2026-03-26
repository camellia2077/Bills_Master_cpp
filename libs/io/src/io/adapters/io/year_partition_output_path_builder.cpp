// io/adapters/io/year_partition_output_path_builder.cpp
#include "io/adapters/io/year_partition_output_path_builder.hpp"

#include <system_error>

namespace fs = std::filesystem;

YearPartitionOutputPathBuilder::YearPartitionOutputPathBuilder(
    std::string base_output_dir)
    : base_output_dir_(std::move(base_output_dir)) {}

auto YearPartitionOutputPathBuilder::build_output_path(
    const fs::path& input_file) const -> fs::path {
  fs::path output_file = input_file;
  output_file.replace_extension(".json");
  const std::string stem = output_file.stem().string();

  fs::path target_dir = base_output_dir_;
  if (stem.size() >= 4U) {
    target_dir /= stem.substr(0, 4);
  }

  std::error_code create_error;
  fs::create_directories(target_dir, create_error);

  return target_dir / output_file.filename();
}
