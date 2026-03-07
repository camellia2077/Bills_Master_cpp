// windows/presentation/cli/command_handler/commands/export_command.hpp
#ifndef WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_EXPORT_COMMAND_H_
#define WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_EXPORT_COMMAND_H_

#include "command_handler/commands/interface/i_command.hpp"

class ExportCommand : public ICommand {
 public:
  // 通过构造函数接收全局选项
  ExportCommand(std::string format, std::string type_filter,
                std::string export_pipeline);
  bool execute(const std::vector<std::string>& args,
               AppController& controller) override;

 private:
  bool handle_export_all(AppController& controller);
  bool handle_export_date(const std::vector<std::string>& values,
                          AppController& controller);

  std::string m_format_str;
  std::string m_type_filter;
  std::string m_export_pipeline;
};

#endif  // WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_EXPORT_COMMAND_H_
