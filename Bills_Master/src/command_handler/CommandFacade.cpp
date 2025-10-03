// command_handler/CommandFacade.cpp
// command_handler/CommandFacade.cpp
#include "CommandFacade.hpp"
#include "app_controller/AppController.hpp"
#include "common/common_utils.hpp"

#include "usage_help.hpp"

#include <iostream>
#include <print>
#include <stdexcept>

void CommandFacade::parse_arguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" || arg == "-f") {
            if (i + 1 < argc) {
                m_format_str = argv[++i];
            } else {
                throw std::runtime_error("Missing value for format flag.");
            }
        } else if (arg == "--type" || arg == "-t") {
            if (i + 1 < argc) {
                m_export_type_filter = argv[++i];
            } else {
                throw std::runtime_error("Missing value for --type flag.");
            }
        } else {
            m_command_parts.push_back(arg);
        }
    }
}

bool CommandFacade::handle_export_command() {
    AppController controller;
    std::vector<std::string> values;
    std::string command = m_command_parts[0];
     if (m_command_parts.size() > 1) {
        command += " " + m_command_parts[1];
        values.assign(m_command_parts.begin() + 2, m_command_parts.end());
    }

    if (command == "--export all" || command == "-e a") {
        std::vector<std::string> formats_to_process;
        if (m_format_str == "all" || m_format_str == "a") {
            formats_to_process = {"md", "tex", "rst", "typ"};
            std::println("\n--- Batch export for all formats requested ---");
        } else {
            formats_to_process.push_back(m_format_str);
        }

        bool all_successful = true;
        for (const auto& current_format : formats_to_process) {
            std::string export_target = "all";
            if (m_export_type_filter == "month" || m_export_type_filter == "m") {
                export_target = "all_months";
            } else if (m_export_type_filter == "year" || m_export_type_filter == "y") {
                export_target = "all_years";
            } else if (!m_export_type_filter.empty()) {
                throw std::runtime_error("Unknown value for --type: '" + m_export_type_filter + "'. Use 'month'/'m' or 'year'/'y'.");
            }

            if (formats_to_process.size() > 1) {
                std::println("\n-> Processing format: {}", current_format);
            }

            if (!controller.handle_export(export_target, {}, current_format)) {
                all_successful = false;
            }
        }
        return all_successful;
    }

    if (command == "--export date" || command == "-e d") {
        if (values.empty()) {
            throw std::runtime_error("Missing date value(s) for 'export date' command.");
        }
        return controller.handle_export("date", values, m_format_str);
    }
    
    // 如果命令不匹配，返回 false
    return false;
}


bool CommandFacade::execute_command() {
    if (m_command_parts.empty()) {
        print_help("bills_master"); // 默认程序名
        return false;
    }

    AppController controller;
    std::string command = m_command_parts[0];
    std::vector<std::string> values(m_command_parts.begin() + 1, m_command_parts.end());

    if (command == "--help" || command == "-h") {
        print_help("bills_master");
        return true;
    }
    if (command == "--version" || command == "-V") {
        controller.display_version();
        return true;
    }
    // 将导出命令的处理委托给专门的函数
    if (command == "--export" || command == "-e") {
        return handle_export_command();
    }
    if (command == "--validate" || command == "-v") {
        if (values.empty()) throw std::runtime_error("Missing path for 'validate' command.");
        return controller.handle_validation(values[0]);
    }
    if (command == "--modify" || command == "-m") {
        if (values.empty()) throw std::runtime_error("Missing path for 'modify' command.");
        return controller.handle_modification(values[0]);
    }
    if (command == "--import" || command == "-i") {
        if (values.empty()) throw std::runtime_error("Missing path for 'import' command.");
        return controller.handle_import(values[0]);
    }
    if (command == "--query" || command == "-q") {
         if (m_command_parts.size() < 2) {
            throw std::runtime_error("Incomplete query command. Use 'query year <YYYY>' or 'query month <YYYYMM>'.");
        }
        std::string query_type = m_command_parts[1];
        values.assign(m_command_parts.begin() + 2, m_command_parts.end());

        if (query_type == "year" || query_type == "y") {
            if (values.empty()) throw std::runtime_error("Missing <year> for 'query year' command.");
            return controller.handle_export("year", values, m_format_str);
        }
        if (query_type == "month" || query_type == "m") {
            if (values.empty()) throw std::runtime_error("Missing <month> for 'query month' command.");
            return controller.handle_export("month", values, m_format_str);
        }
    }
    
    // 如果没有匹配的命令
    std::println(stderr, "{}Error: {}Unknown or incomplete command.\n", RED_COLOR, RESET_COLOR);
    print_help("bills_master");
    return false;
}

int CommandFacade::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    
    try {
        parse_arguments(argc, argv);
        if (execute_command()) {
            return 0; // 成功
        } else {
            return 1; // 失败
        }
    } catch (const std::exception& e) {
        std::println(stderr, "\n{}Critical Error: {}{}", RED_COLOR, RESET_COLOR, e.what());
        return 1;
    }
}