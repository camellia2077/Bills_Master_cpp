#include "presentation/cli_app.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <CLI/CLI.hpp>

#include "bill_generator.h"
#include "config_io.h"
#include "file_io.h"

namespace {

constexpr char kGeneratorConfigName[] = "config.toml";
constexpr char kGeneratorVersion[] = "1.3.0";
constexpr char kGeneratorLastUpdate[] = "2026-03-27";
constexpr char kOutputDirectoryName[] = "bills_output_from_config";

struct GeneratorRequest {
  int start_year = 0;
  int end_year = 0;
};

auto MakeVersionText() -> std::string {
  return std::string("generator version ") + kGeneratorVersion +
         "\nLast updated: " + kGeneratorLastUpdate + '\n';
}

auto MakeExamplesText() -> std::string {
  return "Examples:\n"
         "  generator --single 2024\n"
         "  generator --double 2024 2025\n";
}

auto BuildRequest(const std::optional<int>& single_year,
                  const std::vector<int>& double_years)
    -> std::optional<GeneratorRequest> {
  if (single_year.has_value()) {
    return GeneratorRequest{*single_year, *single_year};
  }
  if (double_years.size() == 2U) {
    return GeneratorRequest{double_years.front(), double_years.back()};
  }
  return std::nullopt;
}

auto RunGenerator(const GeneratorRequest& request) -> int {
  GeneratorConfigData config_data;
  std::string error_message;
  if (!load_generator_config(kGeneratorConfigName, config_data, error_message)) {
    std::cerr << "Error: " << error_message << std::endl;
    return 1;
  }

  BillGenerator generator(std::move(config_data.categories),
                          config_data.comment_probability,
                          std::move(config_data.comments),
                          std::move(config_data.remark_summary_lines),
                          std::move(config_data.remark_followup_lines));

  const std::filesystem::path base_output_dir(kOutputDirectoryName);
  if (!ensure_directory(base_output_dir, error_message)) {
    std::cerr << "Fatal: " << error_message << std::endl;
    return 1;
  }

  std::cout << "Configuration loaded. Generating bill files from "
            << request.start_year << " to " << request.end_year << "..."
            << std::endl;

  const auto start_time = std::chrono::high_resolution_clock::now();
  int generated_file_count = 0;

  for (int year = request.start_year; year <= request.end_year; ++year) {
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

}  // namespace

auto CliApp::Run(int argc, char* argv[]) -> int {
  try {
    CLI::App app(
        "A pseudo-random bill file generator for fixture data generation.");
    app.footer(MakeExamplesText());
    app.set_help_flag("-h,--help", "Show this help message and exit.");
    app.set_version_flag("--version", MakeVersionText(),
                         "Show version information and exit.");

    std::optional<int> single_year;
    std::vector<int> double_years;

    auto* single_option = app.add_option(
        "-s,--single", single_year,
        "Generate bills for a single specified year.");
    auto* double_option = app.add_option(
        "-d,--double", double_years,
        "Generate bills for all years in the inclusive range.");
    double_option->expected(2);

    single_option->excludes(double_option);
    double_option->excludes(single_option);
    app.require_option(1);

    try {
      app.parse(argc, argv);
    } catch (const CLI::ParseError& error) {
      return app.exit(error);
    }

    const auto request = BuildRequest(single_year, double_years);
    if (!request.has_value()) {
      std::cerr << "Error: no generation request was provided." << std::endl;
      return 1;
    }
    if (request->start_year > request->end_year) {
      std::cerr << "Error: Start year cannot be after end year." << std::endl;
      return 1;
    }

    return RunGenerator(*request);
  } catch (const std::exception& error) {
    std::cerr << "Fatal: " << error.what() << std::endl;
    return 1;
  }
}
