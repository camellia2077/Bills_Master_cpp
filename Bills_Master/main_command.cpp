#include "common/pch.h"
#include <iostream>
#include <string>
#include <vector>
#include <print>

#include "app_controller/AppController.h"
#include "common/common_utils.h" // for colors
#include "usage_help/usage_help.h" //
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

// The print_help function has been moved to usage_help.cpp

int main(int argc, char* argv[]) {
    setup_console();
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    AppController controller;
    bool operation_successful = true;
    
    // --- ** 修改：采用新的参数解析逻辑 ** ---
    std::vector<std::string> command_parts;
    std::string format_str = "md";
    std::string export_type_filter;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" || arg == "-f") {
            if (i + 1 < argc) { format_str = argv[++i]; }
            else { std::cerr << "Error: Missing value for format flag.\n"; return 1; }
        } else if (arg == "--type" || arg == "-t") {
            if (i + 1 < argc) { export_type_filter = argv[++i]; }
            else { std::cerr << "Error: Missing value for --type flag.\n"; return 1; }
        } else {
            command_parts.push_back(arg);
        }
    }

    if (command_parts.empty()) {
        print_help(argv[0]);
        return 1;
    }

    std::string command = command_parts[0];
    std::vector<std::string> values;

    // 组合主命令和子命令 (e.g., "-e" + "d" -> "-e d")
    if (command_parts.size() > 1 && (command == "-q" || command == "--query" || command == "-e" || command == "--export" || command == "-a" || command == "--all")) {
        command += " " + command_parts[1];
        values.assign(command_parts.begin() + 2, command_parts.end());
    } else {
        values.assign(command_parts.begin() + 1, command_parts.end());
    }
    
    try {
        if (command == "--help" || command == "-h") {
            print_help(argv[0]);
        } 
        else if (command == "--version" || command == "-V") {
            controller.display_version();
        }
        else if (command == "--export all" || command == "-e a") {
            std::string export_target = "all";
            if (export_type_filter == "month" || export_type_filter == "m") {
                export_target = "all_months";
            } else if (export_type_filter == "year" || export_type_filter == "y") {
                export_target = "all_years";
            } else if (!export_type_filter.empty()) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown value for --type: '" << export_type_filter << "'. Use 'month'/'m' or 'year'/'y'.\n";
                return 1;
            }
            if (!controller.handle_export(export_target, {}, format_str)) operation_successful = false;
        }
        // --- ** 修改：处理 --export date 及其多值参数 ** ---
        else if (command == "--export date" || command == "-e d") {
            if (values.empty()) { 
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing date value(s) for 'export date' command.\n"; 
                return 1; 
            }
            // `values` 向量已包含所有日期参数，直接传递给 controller
            if (!controller.handle_export("date", values, format_str)) {
                operation_successful = false;
            }
        }
        else if (command == "--validate" || command == "-v") {
            if (values.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'validate' command.\n"; return 1; }
            if (!controller.handle_validation(values[0])) operation_successful = false;
        }
        else if (command == "--modify" || command == "-m") {
            if (values.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'modify' command.\n"; return 1; }
            if (!controller.handle_modification(values[0])) operation_successful = false;
        }
        else if (command == "--import" || command == "-i") {
            if (values.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'import' command.\n"; return 1; }
            if (!controller.handle_import(values[0])) operation_successful = false;
        }
        else if (command == "--query year" || command == "-q y") {
            if (values.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> for 'query year' command.\n"; return 1; }
            if (!controller.handle_export("year", values, format_str)) operation_successful = false;
        }
        else if (command == "--query month" || command == "-q m") {
           if (values.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <month> for 'query month' command.\n"; return 1; }
           if (!controller.handle_export("month", values, format_str)) operation_successful = false;
        }
        else {
            std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown or incomplete command.\n\n";
            print_help(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n" << RED_COLOR << "Critical Error: " << RESET_COLOR << e.what() << std::endl; 
        return 1;
    }

    return operation_successful ? 0 : 1; 
}