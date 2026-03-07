// windows/presentation/cli/command_handler/commands/query_command.hpp
#ifndef WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_QUERY_COMMAND_H_
#define WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_QUERY_COMMAND_H_

#include "command_handler/commands/interface/i_command.hpp"

class QueryCommand : public ICommand {
 public:
  QueryCommand(std::string format, std::string export_pipeline);
  bool execute(const std::vector<std::string>& args,
               AppController& controller) override;

 private:
  std::string m_format_str;
  std::string m_export_pipeline;
};

#endif  // WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_QUERY_COMMAND_H_
