// windows/presentation/cli/command_handler/commands/interface/i_command.hpp
#ifndef WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_INTERFACE_I_COMMAND_H_
#define WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_INTERFACE_I_COMMAND_H_

#include <string>
#include <vector>

class AppController;

class ICommand {
 public:
  virtual ~ICommand() = default;

  /**
   * @brief 执行命令。
   * @param args 命令自身的参数 (不包含命令名)。
   * @param controller 用于执行业务逻辑的 AppController 实例。
   * @return 如果成功则返回 true，否则返回 false。
   */
  virtual bool execute(const std::vector<std::string>& args,
                       AppController& controller) = 0;
};

#endif  // WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_INTERFACE_I_COMMAND_H_
