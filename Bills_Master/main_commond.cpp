#include <iostream>
#include <string>

#include "AppController.h"
#include "common_utils.h"

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
    std::cout << "Bill Reprocessor - A command-line tool for processing bill files.\n\n";
    std::cout << "Usage: " << program_name << " <command> [arguments...]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  process, -p <path>          Run the full workflow (validate, modify, import) on a file or directory.\n";
    std::cout << "  export-all, -ea             Export all yearly and monthly reports from the database.\n"; // MODIFIED: New command
    std::cout << "  validate, -v <path>         Validate a .txt bill file or all .txt files in a directory.\n";
    std::cout << "  modify, -m <path>           Modify a .txt file or all .txt files in a directory.\n";
    std::cout << "  import, -i <path>           Parse and insert a .txt file or a directory of .txt files into the database.\n";
    std::cout << "  query-year, -qy <year>      Query the annual summary for the given year (e.g., 2025).\n";
    std::cout << "  query-month, -qm <month>    Query the detailed monthly bill for the given month (e.g., 202507).\n\n";
    std::cout << "Options:\n";
    std::cout << "  --version, -V               Display application version information.\n";
    std::cout << "  --help, -h                  Display this help message.\n";
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
        else if (command == "export-all" || command == "-ea") { 
            controller.handle_export_all();
        }
        else if (command == "process" || command == "-p") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'process' command.\n"; return 1; }
            controller.handle_full_workflow(argv[2]);
        }
        else if (command == "validate" || command == "-v") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'validate' command.\n"; return 1; }
            controller.handle_validation(argv[2]);
        }
        else if (command == "modify" || command == "-m") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'modify' command.\n"; return 1; }
            controller.handle_modification(argv[2]);
        }
        else if (command == "import" || command == "-i") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'import' command.\n"; return 1; }
            controller.handle_import(argv[2]);
        }
        else if (command == "query-year" || command == "-qy") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> argument for 'query-year' command.\n"; return 1; }
            controller.handle_yearly_query(argv[2]);
        }
        else if (command == "query-month" || command == "-qm") {
             if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <month> argument for 'query-month' command.\n"; return 1; }
            controller.handle_monthly_query(argv[2]);
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
