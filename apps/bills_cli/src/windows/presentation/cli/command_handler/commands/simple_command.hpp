// windows/presentation/cli/command_handler/commands/simple_command.hpp
#ifndef WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_SIMPLE_COMMAND_H_
#define WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_SIMPLE_COMMAND_H_

#include <functional>

#include "command_handler/commands/interface/i_command.hpp"

// 一个用于处理简单命令（如 validate, modify, import）的通用类
class SimpleCommand : public ICommand {
 public:
  using Action = std::function<bool(AppController&, const std::string&)>;

  SimpleCommand(std::string command_name, Action action);
  bool execute(const std::vector<std::string>& args,
               AppController& controller) override;

 private:
  std::string m_command_name;
  Action m_action;
};

#endif  // WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_SIMPLE_COMMAND_H_