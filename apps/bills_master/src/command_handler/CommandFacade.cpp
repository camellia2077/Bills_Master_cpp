// command_handler/CommandFacade.cpp
#include "CommandFacade.hpp"
#include "app_controller/AppController.hpp"
#include "common/common_utils.hpp"
#include "usage_help.hpp"

// 引入所有具体的命令类
#include "commands/SimpleCommand.hpp"
#include "commands/ExportCommand.hpp"
#include "commands/QueryCommand.hpp"

#include <iostream>
#include <print>
#include <stdexcept>

void CommandFacade::register_commands(const std::string& format, const std::string& type_filter) {
    // 注册简单命令
    m_commands["--validate"] = std::make_unique<SimpleCommand>("validate", &AppController::handle_validation);
    m_commands["-v"] = std::make_unique<SimpleCommand>("validate", &AppController::handle_validation);
    
    m_commands["--modify"] = std::make_unique<SimpleCommand>("modify", &AppController::handle_modification);
    m_commands["-m"] = std::make_unique<SimpleCommand>("modify", &AppController::handle_modification);

    m_commands["--import"] = std::make_unique<SimpleCommand>("import", &AppController::handle_import);
    m_commands["-i"] = std::make_unique<SimpleCommand>("import", &AppController::handle_import);

    // [新增] 注册完整工作流命令
    m_commands["--full-workflow"] = std::make_unique<SimpleCommand>("full-workflow", &AppController::handle_full_workflow);
    m_commands["-F"] = std::make_unique<SimpleCommand>("full-workflow", &AppController::handle_full_workflow);

    // 注册复杂命令，并传入全局选项
    m_commands["--export"] = std::make_unique<ExportCommand>(format, type_filter);
    m_commands["-e"] = std::make_unique<ExportCommand>(format, type_filter);

    m_commands["--query"] = std::make_unique<QueryCommand>(format);
    m_commands["-q"] = std::make_unique<QueryCommand>(format);
}

int CommandFacade::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    try {
        std::vector<std::string> args(argv + 1, argv + argc);
        std::string command_name;
        std::vector<std::string> command_args;
        std::string format_str = "md";
        std::string type_filter;

        // 1. 解析全局选项和命令
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];
            if (arg == "--format" || arg == "-f") {
                if (++i < args.size()) format_str = args[i];
                else throw std::runtime_error("Missing value for format flag.");
            } else if (arg == "--type" || arg == "-t") {
                if (++i < args.size()) type_filter = args[i];
                else throw std::runtime_error("Missing value for --type flag.");
            } else if (command_name.empty()) {
                command_name = arg;
            } else {
                command_args.push_back(arg);
            }
        }

        // 处理内置的 help 和 version 命令
        if (command_name == "--help" || command_name == "-h") {
            print_help(argv[0]);
            return 0;
        }
        if (command_name == "--version" || command_name == "-V") {
            AppController().display_version();
            return 0;
        }
        
        // 2. 注册所有命令
        register_commands(format_str, type_filter);

        // 3. 查找并执行命令
        auto it = m_commands.find(command_name);
        if (it != m_commands.end()) {
            AppController controller; // 仅在需要执行命令时创建
            if (it->second->execute(command_args, controller)) {
                return 0; // 成功
            }
            return 1; // 失败
        }

        // 4. 如果命令未找到
        std::println(stderr, "{}Error: {}Unknown or incomplete command '{}'.\n", RED_COLOR, RESET_COLOR, command_name);
        print_help(argv[0]);
        return 1;

    } catch (const std::exception& e) {
        std::println(stderr, "\n{}Critical Error: {}{}", RED_COLOR, RESET_COLOR, e.what());
        return 1;
    }
}