// windows/presentation/cli/command_handler/usage_help.cpp

#include "usage_help.hpp"

#include <iostream>
#include <print>
#include <string>

#include "common/common_utils.hpp"  // Required for color codes

namespace terminal = common::terminal;

void print_help(const char* program_name) {
  std::string command_color(terminal::kCyan);

  std::println(
      "Bill Master - A command-line tool for processing bill files.\n");
  std::println(
      "Usage: {} <command> [arguments] [--format <format>] [--type <type>] "
      "[--export-pipeline <legacy|model-first|json-first>]\n",
      program_name);

  std::cout << terminal::kGreen << "--- Core Commands ---\n" << terminal::kReset;

  std::println("{}--validate, -v{} <path>", command_color, terminal::kReset);
  std::println(
      "  Validates the format of one or more bill files at the given path.\n");

  std::println("{}--modify, -m{} <path>", command_color, terminal::kReset);
  std::println("  Converts bill files to JSON (legacy alias of --convert).\n");

  std::println("{}--convert, -c{} <path>", command_color, terminal::kReset);
  std::println("  Converts bill files to JSON for persistence.\n");

  std::println("{}--import, -i{} <path>", command_color, terminal::kReset);
  std::println("  Parses and inserts bill data into the database.\n");

  std::println("{}--ingest, -I{} <path> [--json|-j]", command_color,
               terminal::kReset);
  std::println(
      "  Validates, converts, and inserts bill files. Use --json to persist "
      "JSON locally.\n");

  // [新增] 添加 full-workflow 的帮助说明
  std::println("{}--full-workflow, -F{} <path>", command_color, terminal::kReset);
  std::println(
      "  Runs the full workflow (validate, convert, import) for bill files.\n");

  std::cout << terminal::kGreen << "--- Query & Export ---\n" << terminal::kReset;

  std::println("{}--query year, -q y{} <year>", command_color, terminal::kReset);
  std::println("  Queries and exports the annual summary.");
  std::println("  Example: {} -q y 2024 --format tex\n", program_name);

  std::println("{}--query month, -q m{} <month>", command_color, terminal::kReset);
  std::println("  Queries and exports the monthly details (format: YYYY-MM).");
  std::println("  Example: {} -q m 2024-07\n", program_name);

  std::println("{}--export all, -e a{}", command_color, terminal::kReset);
  std::println(
      "  Exports all reports. Use optional --type <month|year> to filter.");
  std::println("  Example: {} -e a --type month --format tex\n", program_name);

  std::println("{}--export date, -e d{} <date1> [date2]", command_color,
               terminal::kReset);
  std::println("  Exports reports for a specific date or date range.");
  std::println(
      "  - Single date: YYYY (all months in year), YYYY-MM (single month).");
  std::println(
      "  - Date range: YYYY-MM YYYY-MM (all months between start and end, "
      "inclusive).");
  std::println("  Example (single): {} -e d 2024 -f tex", program_name);
  std::println("  Example (range):  {} -e d 2024-04 2024-07 -f md\n",
               program_name);

  std::cout << terminal::kGreen << "--- Options ---\n" << terminal::kReset;

  std::println("  --format, -f <format>");
  std::println(
      "    Specify output format ('md', 'json', 'tex', 'rst', 'all', 'a'). "
      "Defaults to 'md'.");
  std::println("    Use 'all' or 'a' to export in all available formats.");
  std::println("    Example: --format tex");

  std::println("  --type, -t <type>");
  std::println(
      "    For '--export all', filters by type ('month'/'m' or 'year'/'y').");
  std::println("    Example: --type year");

  std::println("  --export-pipeline <pipeline>");
  std::println(
      "    Select report export pipeline "
      "('legacy', 'model-first', or 'json-first'). "
      "Defaults to 'model-first'.");
  std::println("    Example: --export-pipeline model-first\n");

  std::cout << terminal::kGreen << "--- General ---\n" << terminal::kReset;

  std::println("  -h, --help");
  std::println("    Shows this help message.");

  std::println("  -V, --version");
  std::println("    Shows the program version.");
}

