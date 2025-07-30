#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include "_internal/arg_parser.h"
#include "_internal/BillGenerator.h"
#include "_internal/utils.h"

int main(int argc, char* argv[]) {
    // --- 1. Argument Parsing ---
    const ProgramOptions options = parse_arguments(argc, argv);

    // Handle immediate actions like help, version, or errors from parsing
    if (options.action == Action::SHOW_HELP) {
        show_help(argv[0]);
        return 0;
    }
    if (options.action == Action::SHOW_VERSION) {
        const std::string APP_VERSION = "1.3.0";
        const std::string LAST_UPDATE = "2025-07-28";
        show_version(APP_VERSION, LAST_UPDATE);
        return 0;
    }
    if (options.action == Action::ERROR) {
        std::cerr << "Error: " << options.error_message << std::endl;
        show_help(argv[0]);
        return 1;
    }

    // --- 2. Configuration and Generator Setup ---
    const std::string config_filename = "config.json";
    BillGenerator generator;
    
    if (!generator.load_config(config_filename)) {
        // Error messages are printed inside the load_config function
        return 1;
    }

    // --- 3. Main Generation Logic ---
    std::string output_dir_name = "bills_output_from_config";
    std::filesystem::path base_output_dir(output_dir_name);

    try {
        if (!std::filesystem::exists(base_output_dir)) {
            std::filesystem::create_directory(base_output_dir);
            std::cout << "Created base directory: " << output_dir_name << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Fatal: Failed to create base directory: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Configuration loaded. Generating bill files from " << options.start_year << " to " << options.end_year << "..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int year = options.start_year; year <= options.end_year; ++year) {
        std::filesystem::path year_dir = base_output_dir / std::to_string(year);
        try {
            if (!std::filesystem::exists(year_dir)) {
                std::filesystem::create_directory(year_dir);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: Failed to create directory for year " << year << ": " << e.what() << std::endl;
            continue; // Skip this year
        }

        for (int month = 1; month <= 12; ++month) {
            generator.generate_bill_file(year, month, year_dir);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double total_seconds = duration.count() / 1000.0;
    
    int year_duration = options.end_year - options.start_year + 1;
    int total_files_generated = year_duration * 12;

    // --- 4. Final Summary ---
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "Successfully generated files: " << total_files_generated << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Timing Statistics:" << std::endl;
    std::cout << "Total time: " << std::fixed << std::setprecision(4) << total_seconds << " seconds (" << duration.count() << " ms)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}