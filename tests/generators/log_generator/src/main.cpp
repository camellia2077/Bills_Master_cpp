#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "internal/arg_parser.h"
#include "internal/bill_generator.h"
#include "internal/config_io.h"
#include "internal/file_io.h"
#include "internal/utils.h"

auto main(int argc, char* argv[]) -> int {
  const ProgramOptions options = parse_arguments(argc, argv);
  if (options.action == Action::SHOW_HELP) {
    show_help(argv[0]);
    return 0;
  }
  if (options.action == Action::SHOW_VERSION) {
    const std::string app_version = "1.3.0";
    const std::string last_update = "2025-07-28";
    show_version(app_version, last_update);
    return 0;
  }
  if (options.action == Action::ERROR) {
    std::cerr << "Error: " << options.error_message << std::endl;
    show_help(argv[0]);
    return 1;
  }

  GeneratorConfigData config_data;
  std::string error_message;
  if (!load_generator_config("config.json", config_data, error_message)) {
    std::cerr << "Error: " << error_message << std::endl;
    return 1;
  }

  BillGenerator generator(std::move(config_data.categories),
                          config_data.comment_probability,
                          std::move(config_data.comments));

  const std::filesystem::path base_output_dir("bills_output_from_config");
  if (!ensure_directory(base_output_dir, error_message)) {
    std::cerr << "Fatal: " << error_message << std::endl;
    return 1;
  }

  std::cout << "Configuration loaded. Generating bill files from "
            << options.start_year << " to " << options.end_year << "..."
            << std::endl;

  const auto start_time = std::chrono::high_resolution_clock::now();
  int generated_file_count = 0;

  for (int year = options.start_year; year <= options.end_year; ++year) {
    const std::filesystem::path year_dir = base_output_dir / std::to_string(year);
    if (!ensure_directory(year_dir, error_message)) {
      std::cerr << "Error: " << error_message << std::endl;
      continue;
    }

    for (int month = 1; month <= 12; ++month) {
      std::ostringstream file_name;
      file_name << year << "-" << std::setw(2) << std::setfill('0') << month
                << ".txt";
      const std::filesystem::path output_file = year_dir / file_name.str();
      const std::string bill_content = generator.generate_bill_content(year, month);
      if (!write_text_file(output_file, bill_content, error_message)) {
        std::cerr << "Error: " << error_message << std::endl;
        continue;
      }
      std::cout << "Successfully generated bill file: " << output_file
                << std::endl;
      ++generated_file_count;
    }
  }

  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);
  const double total_seconds = duration.count() / 1000.0;

  std::cout << "\n----------------------------------------" << std::endl;
  std::cout << "Successfully generated files: " << generated_file_count
            << std::endl;
  std::cout << "----------------------------------------" << std::endl;
  std::cout << "Timing Statistics:" << std::endl;
  std::cout << "Total time: " << std::fixed << std::setprecision(4)
            << total_seconds << " seconds (" << duration.count() << " ms)"
            << std::endl;
  std::cout << "----------------------------------------" << std::endl;

  return 0;
}
