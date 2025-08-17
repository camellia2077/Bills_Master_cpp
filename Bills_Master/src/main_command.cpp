#include "common/pch.h"
#include <string>
#include <vector>
#include <print>
#include <cstdio> // 需要包含 <cstdio> 来获取 stderr

#include "app_controller/AppController.h"
#include "common/common_utils.h" // for colors
#include "usage_help/usage_help.h" // for help

// Add to the top of main.cpp
#ifdef _WIN32
#include <windows.h>
#endif

// Add this helper function before main()
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
    
    std::vector<std::string> command_parts;
    std::string format_str = "md";
    std::string export_type_filter;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" || arg == "-f") {
            if (i + 1 < argc) { format_str = argv[++i]; }
            // 修正：修复了拼写错误、多余的分号和换行符
            else { std::println(stderr, "{}Error{}: Missing value for format flag", RED_COLOR, RESET_COLOR); return 1; }
        } else if (arg == "--type" || arg == "-t") {
            if (i + 1 < argc) { export_type_filter = argv[++i]; }
            // 修改：将 std::cerr 替换为 std::println
            else { std::println(stderr, "Error: Missing value for --type flag."); return 1; }
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
            std::vector<std::string> formats_to_process;
            if (format_str == "all" || format_str == "a") {
                formats_to_process = {"md", "tex", "rst", "typ"};
                std::println("\n--- Batch export for all formats requested ---");
            } else {
                formats_to_process.push_back(format_str);
            }

            for (const auto& current_format : formats_to_process) {
                std::string export_target = "all";
                if (export_type_filter == "month" || export_type_filter == "m") {
                    export_target = "all_months";
                } else if (export_type_filter == "year" || export_type_filter == "y") {
                    export_target = "all_years";
                } else if (!export_type_filter.empty()) {
                    // 修正：修复了格式字符串中的错误和多余的换行符
                    std::println(stderr, "{}Error{}: Unknown value for --type: '{}'. Use 'month'/'m' or 'year'/'y'.", RED_COLOR, RESET_COLOR, export_type_filter);
                    return 1;
                }

                if (formats_to_process.size() > 1) {
                    std::println("\n-> Processing format: {}", current_format);
                }

                if (!controller.handle_export(export_target, {}, current_format)) {
                    operation_successful = false; 
                }
            }
        }
        else if (command == "--export date" || command == "-e d") {
            if (values.empty()) { 
                std::println(stderr, "{}Error{}: Missing date value(s) for 'export date' command.", RED_COLOR, RESET_COLOR);
                return 1; 
            }
            if (!controller.handle_export("date", values, format_str)) {
                operation_successful = false;
            }
        }
        else if (command == "--validate" || command == "-v") {
            // 修改：将 std::cerr 替换为 std::println
            if (values.empty()) { std::println(stderr, "{}Error: {}Missing path for 'validate' command.", RED_COLOR, RESET_COLOR); return 1; }
            if (!controller.handle_validation(values[0])) operation_successful = false;
        }
        else if (command == "--modify" || command == "-m") {
            // 修改：将 std::cerr 替换为 std::println
            if (values.empty()) { std::println(stderr, "{}Error: {}Missing path for 'modify' command.", RED_COLOR, RESET_COLOR); return 1; }
            if (!controller.handle_modification(values[0])) operation_successful = false;
        }
        else if (command == "--import" || command == "-i") {
            // 修改：将 std::cerr 替换为 std::println
            if (values.empty()) { std::println(stderr, "{}Error: {}Missing path for 'import' command.", RED_COLOR, RESET_COLOR); return 1; }
            if (!controller.handle_import(values[0])) operation_successful = false;
        }
        else if (command == "--query year" || command == "-q y") {
            // 修改：将 std::cerr 替换为 std::println
            if (values.empty()) { std::println(stderr, "{}Error: {}Missing <year> for 'query year' command.", RED_COLOR, RESET_COLOR); return 1; }
            if (!controller.handle_export("year", values, format_str)) operation_successful = false;
        }
        else if (command == "--query month" || command == "-q m") {
            // 修改：将 std::cerr 替换为 std::println
           if (values.empty()) { std::println(stderr, "{}Error: {}Missing <month> for 'query month' command.", RED_COLOR, RESET_COLOR); return 1; }
           if (!controller.handle_export("month", values, format_str)) operation_successful = false;
        }
        else {
            // 修改：将 std::cerr 替换为 std::println，并保留一个换行符
            std::println(stderr, "{}Error: {}Unknown or incomplete command.\n", RED_COLOR, RESET_COLOR);
            print_help(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        // 修改：将 std::cerr 替换为 std::println
        std::println(stderr, "\n{}Critical Error: {}{}", RED_COLOR, RESET_COLOR, e.what()); 
        return 1;
    }

    return operation_successful ? 0 : 1; 
}