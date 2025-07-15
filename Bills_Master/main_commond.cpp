#include <iostream>
#include <string>
#include <vector>
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

// 更新帮助信息以包含 --format 标志
void print_help(const char* program_name) {

    std::println("Bill Master - A command-line tool for processing bill files.\n\n");
    std::println("Usage: {} <command> [arguments] [--format <md|tex>]\n", program_name);

    std::cout << GREEN_COLOR << "--- Reprocessor ---\n" << RESET_COLOR;
    std::println("--validate, -v <path> \t\tValidate a .txt bill file or all .txt files in a directory.");
    std::println("--modify, -m <path> \t\tModify a .txt file or all .txt files in a directory.");

    std::cout << GREEN_COLOR << "--- DB Insertor ---\n" << RESET_COLOR;
    std::println("--import, -i <path> \t\tParse and insert a .txt file or a directory into the database.");
    std::println("--process, -p <path> \t\tRun the full workflow (validate, modify, import)");

    std::cout << GREEN_COLOR << "--- Query & Export ---\n" << RESET_COLOR;
    std::println("--query year, -q y <year> \t\tQuery the annual summary for the given year and export (e.g., 2025)");
    std::println("--query month, -q m <month> \t\tQuery the detailed monthly bill for the given month and export (e.g., 202507).");
    std::println("--export all, -e a \t\t\tExport all yearly and monthly reports from the database.");

    std::cout << GREEN_COLOR << "--- Options ---\n" << RESET_COLOR;
    std::println("  --format, -f <format>\t\tSpecify the output format ('md' or 'tex'). Default is 'md'.");

    std::cout << GREEN_COLOR << "--- General ---\n" << RESET_COLOR;
    std::println("  -h, --help\t\t\tShow this help message.");
    std::println("  -v, --version\t\t\tShow program version.\n");
}

int main(int argc, char* argv[]) {
    setup_console();

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    AppController controller;
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string command;
    std::string path_or_value;
    std::string format_str = "md"; // 默认格式为 markdown

    // --- 解析参数，分离出命令、值和格式 ---
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--format" || args[i] == "-f") {
            if (i + 1 < args.size()) {
                format_str = args[i + 1];
                i++; // 跳过格式值
            } else {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing value for format flag.\n";
                return 1;
            }
        } else if (command.empty()) {
            command = args[i];
        } else if (path_or_value.empty()) {
             // 处理 "query year" 或 "query month" 这种两段式命令
            if ((command == "--query" || command == "-q") && (args[i] == "year" || args[i] == "month" || args[i] == "y" || args[i] == "m")) {
                command += " " + args[i];
            } else {
                 path_or_value = args[i];
            }
        } else if ((command == "--query year" || command == "-q y" || command == "--query month" || command == "-q m") && path_or_value.empty()){
            path_or_value = args[i];
        }
    }
    
    // 如果 path_or_value 仍然为空，但 args 中还有未处理的参数，则它是 path_or_value
    if (path_or_value.empty() && args.size() > 1 && (args[0] != "--help" && args[0] != "-h" && args[0] != "--version" && args[0] != "-V")) {
        if (args.size() > 1 && (args[1] != "--format" && args[1] != "-f")){
           size_t value_index = 1;
            if((command == "--query" || command == "-q") && args.size() > 2) value_index = 2;
            if(args.size() > value_index) path_or_value = args[value_index];
        }
    }


    try {
        if (command == "--help" || command == "-h") {
            print_help(argv[0]);
        } 
        else if (command == "--version" || command == "-V") {
            controller.display_version();
        }
        else if (command == "--export all" || command == "-e a") {
             controller.handle_export("all", "", format_str);
        }
        else if (command == "--process" || command == "-p") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'process' command.\n"; return 1; }
            controller.handle_full_workflow(path_or_value);
        }
        else if (command == "--validate" || command == "-v") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'validate' command.\n"; return 1; }
            controller.handle_validation(path_or_value);
        }
        else if (command == "--modify" || command == "-m") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'modify' command.\n"; return 1; }
            controller.handle_modification(path_or_value);
        }
        else if (command == "--import" || command == "-i") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'import' command.\n"; return 1; }
            controller.handle_import(path_or_value);
        }
        else if (command == "--query year" || command == "-q y") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> for 'query year' command.\n"; return 1; }
            controller.handle_export("year", path_or_value, format_str);
        }
        else if (command == "--query month" || command == "-q m") {
           if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <month> for 'query month' command.\n"; return 1; }
           controller.handle_export("month", path_or_value, format_str);
       }
        else {
            std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown or incomplete command '" << command << "'\n\n";
            print_help(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n" << RED_COLOR << "Critical Error: " << RESET_COLOR << e.what() << std::endl; 
        return 1; 
    }

    return 0; 
}