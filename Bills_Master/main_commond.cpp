#include <iostream>
#include <string>
#include <print>

#include "AppController.h"
#include "common_utils.h" // for colors

// For UTF-8 output on Windows
#ifdef _WIN32
#include <windows.h>
#endif

// Sets up the console for proper UTF-8 character display.
void setup_console() { 
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

// Prints the help message with all available commands
void print_help(const char* program_name) {

    std::println("Bill Master - A command-line tool for processing bill files.\n\n");
    std::println("Usage: {} <command> [arguments]\n", program_name);

    std::cout << GREEN_COLOR << "--- Reprocessor ---\n" << RESET_COLOR;
    std::println("--validate, -v <path> \t\tValidate a .txt bill file or all .txt files in a directory.");

    std::println("--modify, -m <path>, -v <path> \t\tModify a .txt file or all .txt files in a directory.");


    std::cout << GREEN_COLOR << "--- DB Insertor ---\n" << RESET_COLOR;

    std::println("--import, -i <path> \t\tParse and insert a .txt file or a directory into the database.");
    std::println("--process, -p <path> \t\tRun the full workflow (validate, modify, import)");

    std::cout << GREEN_COLOR << "--- Query ---\n" << RESET_COLOR;

    std::println("--query year, -q y <year> \t\tQuery the annual summary for the given year (e.g., 2025)");
    std::println("--query month, -q m <month> \t\tQuery the detailed monthly bill for the given month (e.g., 202507).");

    std::cout << GREEN_COLOR << "--- Export ---\n" << RESET_COLOR;
    std::println("--export all, -e a \t\tExport all yearly and monthly reports from the database.");


    std::cout << GREEN_COLOR << "--- Export ---\n" << RESET_COLOR;


    std::println("  -h, --help\t\t\tShow this help message.\n");
    std::println("  -v, --version\t\t\tShow program version.\n");
}

int main(int argc, char* argv[]) {
    setup_console();

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    AppController controller;
    std::string command = argv[1];

    try {
        if (command == "--help" || command == "-h") {
            print_help(argv[0]);
        } 
        else if (command == "--version" || command == "-V") {
            controller.display_version();
        }
        else if (command == "--export all" || (command == "-e" && argc > 2 && std::string(argv[2]) == "a")) {
            controller.handle_export_all();
        }
        else if (command == "--process" || command == "-p") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'process' command.\n"; return 1; }
            controller.handle_full_workflow(argv[2]);
        }
        else if (command == "--validate" || command == "-v") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'validate' command.\n"; return 1; }
            controller.handle_validation(argv[2]);
        }
        else if (command == "--modify" || command == "-m") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'modify' command.\n"; return 1; }
            controller.handle_modification(argv[2]);
        }
        else if (command == "--import" || command == "-i") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'import' command.\n"; return 1; }
            controller.handle_import(argv[2]);
        }
        else if ((command == "--query" && argc > 2 && std::string(argv[2]) == "year") || (command == "-q" && argc > 2 && std::string(argv[2]) == "y")) {
            // 对于 -q y <year>，年份值是 argv[3]
            if (argc < 4) { // 至少需要4个参数 (程序名, -q, y, <年份>)
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> argument for 'query year' command.\n"; // 更新错误提示
                return 1;
            }
            controller.handle_yearly_query(argv[3]); // 传递 argv[3]
        }
        else if ((command == "--query" && argc > 2 && std::string(argv[2]) == "month") || (command == "-q" && argc > 2 && std::string(argv[2]) == "m")) {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <month> argument for 'query-month' command.\n"; return 1; }
           controller.handle_monthly_query(argv[3]);
       }
        else {
            std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown command '" << command << "'\n\n";
            print_help(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n" << RED_COLOR << "Critical Error: " << RESET_COLOR << e.what() << std::endl; 
        return 1; 
    }

    return 0; 
}
