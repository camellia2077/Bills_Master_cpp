#ifndef WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_RECORD_COMMAND_H_
#define WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_RECORD_COMMAND_H_

#include "command_handler/commands/interface/i_command.hpp"

class RecordCommand : public ICommand {
 public:
  bool execute(const std::vector<std::string>& args,
               AppController& controller) override;
};

#endif  // WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_RECORD_COMMAND_H_
