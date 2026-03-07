// command_handler/CommandDispatcher.cpp
#include "command_dispatcher.hpp"

#include "common/common_utils.hpp"
#include "controllers/app_controller.hpp"
#include "usage_help.hpp"

namespace terminal = common::terminal;

// 引入所有具体的命令类
#include <iostream>
#include <print>
#include <stdexcept>
#include <utility>

#include "commands/export_command.hpp"
#include "commands/ingest_command.hpp"
#include "commands/query_command.hpp"
#include "commands/simple_command.hpp"

auto ICommandDeleter::operator()(ICommand* command) const -> void {
  delete command;
}

void CommandDispatcher::register_commands(const std::string& format,
                                          const std::string& type_filter,
                                          const std::string& export_pipeline) {
  auto make_command = []<typename T, typename... Args>(Args&&... args) -> auto {
    return CommandPtr(new T(std::forward<Args>(args)...));
  };

  // 注册简单命令
  m_commands["--validate"] = make_command.template operator()<SimpleCommand>(
      "validate", &AppController::handle_validation);
  m_commands["-v"] = make_command.template operator()<SimpleCommand>(
      "validate", &AppController::handle_validation);

  m_commands["--modify"] = make_command.template operator()<SimpleCommand>(
      "modify", &AppController::handle_convert);
  m_commands["-m"] = make_command.template operator()<SimpleCommand>(
      "modify", &AppController::handle_convert);

  m_commands["--convert"] = make_command.template operator()<SimpleCommand>(
      "convert", &AppController::handle_convert);
  m_commands["-c"] = make_command.template operator()<SimpleCommand>(
      "convert", &AppController::handle_convert);

  m_commands["--import"] = make_command.template operator()<SimpleCommand>(
      "import", &AppController::handle_import);
  m_commands["-i"] = make_command.template operator()<SimpleCommand>(
      "import", &AppController::handle_import);

  m_commands["--ingest"] = make_command.template operator()<IngestCommand>();
  m_commands["-I"] = make_command.template operator()<IngestCommand>();

  // [新增] 注册完整工作流命令
  m_commands["--full-workflow"] =
      make_command.template operator()<SimpleCommand>(
          "full-workflow", &AppController::handle_full_workflow);
  m_commands["-F"] = make_command.template operator()<SimpleCommand>(
      "full-workflow", &AppController::handle_full_workflow);

  // 注册复杂命令，并传入全局选项
  m_commands["--export"] = make_command.template operator()<ExportCommand>(
      format, type_filter, export_pipeline);
  m_commands["-e"] = make_command.template operator()<ExportCommand>(
      format, type_filter, export_pipeline);

  m_commands["--query"] = make_command.template operator()<QueryCommand>(
      format, export_pipeline);
  m_commands["-q"] = make_command.template operator()<QueryCommand>(
      format, export_pipeline);
}

auto CommandDispatcher::run(int argc, char* argv[]) -> int {
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
    std::string export_pipeline = "model-first";

    // 1. 解析全局选项和命令
    for (size_t i = 0; i < args.size(); ++i) {
      const auto& arg = args[i];
      if (arg == "--format" || arg == "-f") {
        if (++i < args.size()) {
          format_str = args[i];
        } else {
          throw std::runtime_error("Missing value for format flag.");
        }
      } else if (arg == "--type" || arg == "-t") {
        if (++i < args.size()) {
          type_filter = args[i];
        } else {
          throw std::runtime_error("Missing value for --type flag.");
        }
      } else if (arg == "--export-pipeline") {
        if (++i < args.size()) {
          export_pipeline = args[i];
        } else {
          throw std::runtime_error("Missing value for --export-pipeline flag.");
        }
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
      AppController::display_version();
      return 0;
    }

    // 2. 注册所有命令
    register_commands(format_str, type_filter, export_pipeline);

    // 3. 查找并执行命令
    auto command_it = m_commands.find(command_name);
    if (command_it != m_commands.end()) {
      AppController controller;  // 仅在需要执行命令时创建
      if (command_it->second->execute(command_args, controller)) {
        return 0;  // 成功
      }
      return 1;  // 失败
    }

    // 4. 如果命令未找到
    std::println(stderr, "{}Error: {}Unknown or incomplete command '{}'.\n",
                 terminal::kRed, terminal::kReset, command_name);
    print_help(argv[0]);
    return 1;

  } catch (const std::exception& e) {
    std::println(stderr, "\n{}Critical Error: {}{}", terminal::kRed, terminal::kReset,
                 e.what());
    return 1;
  }
}
