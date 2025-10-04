// command_handler/usage_help.cpp

#include "usage_help.hpp"
#include <iostream>
#include <string>
#include <print>

#include "common/common_utils.hpp" // Required for color codes

void print_help(const char* program_name) {
    std::string command_color = CYAN_COLOR, tile_color = GREEN_COLOR;

    std::println("Bill Master - A command-line tool for processing bill files.\n");
    std::println("Usage: {} <command> [arguments] [--format <format>] [--type <type>]\n", program_name);
    
    std::cout << GREEN_COLOR << "--- Core Commands ---\n" << RESET_COLOR;

    std::println("{}--validate, -v{} <path>", command_color, RESET_COLOR);
    std::println("  Validates the format of one or more bill files at the given path.\n");

    std::println("{}--modify, -m{} <path>", command_color, RESET_COLOR);
    std::println("  Applies modifications to bill files based on configuration.\n");

    std::println("{}--import, -i{} <path>", command_color, RESET_COLOR);
    std::println("  Parses and inserts bill data into the database.\n");

    // [新增] 添加 full-workflow 的帮助说明
    std::println("{}--full-workflow, -F{} <path>", command_color, RESET_COLOR);
    std::println("  Runs the full workflow (validate, modify, import) for bill files.\n");

    std::cout << GREEN_COLOR << "--- Query & Export ---\n" << RESET_COLOR;

    std::println("{}--query year, -q y{} <year>", command_color, RESET_COLOR);
    std::println("  Queries and exports the annual summary.");
    std::println("  Example: {} -q y 2024 --format tex\n", program_name);

    std::println("{}--query month, -q m{} <month>", command_color, RESET_COLOR);
    std::println("  Queries and exports the monthly details (format: YYYYMM).");
    std::println("  Example: {} -q m 202407\n", program_name);

    std::println("{}--export all, -e a{}", command_color, RESET_COLOR);
    std::println("  Exports all reports. Use optional --type <month|year> to filter.");
    std::println("  Example: {} -e a --type month --format tex\n", program_name);
    
    std::println("{}--export date, -e d{} <date1> [date2]", command_color, RESET_COLOR);
    std::println("  Exports reports for a specific date or date range.");
    std::println("  - Single date: YYYY (all months in year), YYYYMM (single month).");
    std::println("  - Date range: YYYYMM YYYYMM (all months between start and end, inclusive).");
    std::println("  Example (single): {} -e d 2024 -f tex", program_name);
    std::println("  Example (range):  {} -e d 202404 202407 -f md\n", program_name);

    std::cout << GREEN_COLOR << "--- Options ---\n" << RESET_COLOR;
    
    std::println("  --format, -f <format>");
    std::println("    Specify output format ('md', 'tex', 'typ', 'rst', 'all', 'a'). Defaults to 'md'.");
    std::println("    Use 'all' or 'a' to export in all available formats.");
    std::println("    Example: --format tex");
    
    std::println("  --type, -t <type>");
    std::println("    For '--export all', filters by type ('month'/'m' or 'year'/'y').");
    std::println("    Example: --type year\n");

    std::cout << GREEN_COLOR << "--- General ---\n" << RESET_COLOR;
    
    std::println("  -h, --help");
    std::println("    Shows this help message.");
    
    std::println("  -V, --version");
    std::println("    Shows the program version.");
}