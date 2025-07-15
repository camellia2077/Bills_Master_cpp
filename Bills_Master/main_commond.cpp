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

// 更新帮助信息
void print_help(const char* program_name) {

    std::println("Bill Master - A command-line tool for processing bill files.\n\n");
    std::println("Usage: {} <command> [arguments] [--format <md|tex|typ>]\n", program_name);

    std::cout << GREEN_COLOR << "--- Reprocessor ---\n" << RESET_COLOR;
    std::println("--validate, -v <path> \t\tValidate a .txt bill file or all .txt files in a directory.");
    std::println("--modify, -m <path> \t\tModify a .txt file or all .txt files in a directory.");

    std::cout << GREEN_COLOR << "--- DB Insertor ---\n" << RESET_COLOR;
    std::println("--import, -i <path> \t\tParse and insert a .txt file or a directory into the database.");
    std::println("--process, -p <path> \t\tRun the full workflow (validate, modify, import)");

    std::cout << GREEN_COLOR << "--- Query & Export ---\n" << RESET_COLOR;
    std::println("--query year, -q y <year> \t\tQuery and export the annual summary.");
    std::println("--query month, -q m <month> \t\tQuery and export the monthly details.");
    std::println("--export all, -e a \t\t\tExport all reports. Defaults to all formats unless --format is used.");

    std::cout << GREEN_COLOR << "--- Options ---\n" << RESET_COLOR;
    std::println("  --format, -f <format>\t\tSpecify output format ('md', 'tex', or 'typ'). Default is 'md'.");

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
    std::vector<std::string> args;
    std::string command;
    std::string path_or_value;
    std::string format_str = "md"; // 默认格式
    bool format_specified = false;

    // --- 新的、更可靠的参数解析逻辑 ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" || arg == "-f") {
            if (i + 1 < argc) {
                format_str = argv[++i]; // 获取格式并跳过下一个参数
                format_specified = true;
            } else {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing value for format flag.\n";
                return 1;
            }
        } else {
            args.push_back(arg);
        }
    }

    if (!args.empty()) {
        command = args[0];
        // 检查两段式命令
        if ((command == "--query" || command == "-q" || command == "--export" || command == "-e") && args.size() > 1) {
            // 将 "query year" 或 "export all" 组合成一个命令
            command += " " + args[1];
            if (args.size() > 2) {
                path_or_value = args[2];
            }
        } else if (args.size() > 1) {
            path_or_value = args[1];
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
            if (format_specified) {
                // 如果用户指定了格式，则只导出该格式
                std::cout << "Exporting all reports in " << format_str << " format...\n";
                controller.handle_export("all", "", format_str);
            } else {
                // 如果未指定格式，则导出所有三种格式
                std::cout << "Exporting all reports in Markdown, LaTeX, and Typst formats...\n";
                controller.handle_export("all", "", "md");
                controller.handle_export("all", "", "tex");
                controller.handle_export("all", "", "typ");
            }
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