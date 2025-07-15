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
// 辅助函数，用于打印居中的彩色标题
void print_centered_title(const std::string& text, int total_width) {
    // 计算标题的可见长度（不包括颜色代码）
    const std::string visible_text = "------------ " + text + " ------------";
    const int padding = (total_width - visible_text.length()) / 2;

    // 1. 打印左边距
    std::print("{:>{}}", "", padding); 
    // 2. 打印带颜色的标题
    std::println("--- {}{}{} ---", GREEN_COLOR, text, RESET_COLOR);
}
// Prints the help message with all available commands
void print_help(const char* program_name) {
    const int command_width = 30;
    const int total_width = 80; // 设置一个用于居中的总行宽

    std::println("Bill Reprocessor - A command-line tool for processing bill files.\n\n");
    std::println("Usage: {} <command> [arguments]\n", program_name);

    // --- 使用辅助函数来打印居中的标题 ---
    print_centered_title("Reprocessor", total_width);
    std::println("{:^{}} {}", "validate, -v <path>", command_width, "Validate a .txt bill file or all .txt files in a directory.");
    std::println("{:^{}} {}\n", "modify, -m <path>", command_width, "Modify a .txt file or all .txt files in a directory.");

    print_centered_title("DB Insertor", total_width);
    std::println("{:^{}} {}", "import, -i <path>", command_width, "Parse and insert a .txt file or a directory into the database.");
    std::println("{:^{}} {}\n", "process, -p <path>", command_width, "Run the full workflow (validate, modify, import).");

    print_centered_title("Query", total_width);
    std::println("{:^{}} {}", "query-year, -qy <year>", command_width, "Query the annual summary for the given year (e.g., 2025).");
    std::println("{:^{}} {}\n", "query-month, -qm <month>", command_width, "Query the detailed monthly bill for the given month (e.g., 202507).");

    print_centered_title("Export", total_width);
    std::println("{:^{}} {}\n", "export-all, -ea", command_width, "Export all yearly and monthly reports from the database.");

    print_centered_title("General", total_width);
    std::println("{:^{}} {}", "--version, -V", command_width, "Display application version information.");
    std::println("{:^{}} {}", "--help, -h", command_width, "Display this help message.");
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
        else if (command == "-export all" || command == "-e a") { 
            controller.handle_export_all();
        }
        else if (command == "-process" || command == "-p") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'process' command.\n"; return 1; }
            controller.handle_full_workflow(argv[2]);
        }
        else if (command == "-validate" || command == "-v") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'validate' command.\n"; return 1; }
            controller.handle_validation(argv[2]);
        }
        else if (command == "-modify" || command == "-m") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'modify' command.\n"; return 1; }
            controller.handle_modification(argv[2]);
        }
        else if (command == "-import" || command == "-i") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'import' command.\n"; return 1; }
            controller.handle_import(argv[2]);
        }
        else if (command == "-query year" || command == "-q y") {
            if (argc < 3) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> argument for 'query-year' command.\n"; return 1; }
            controller.handle_yearly_query(argv[2]);
        }
        else if (command == "-query month" || command == "-q m") {
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
