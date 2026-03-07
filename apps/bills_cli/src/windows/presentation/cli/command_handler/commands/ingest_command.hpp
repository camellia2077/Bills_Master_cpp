// windows/presentation/cli/command_handler/commands/ingest_command.hpp
#ifndef WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_INGEST_COMMAND_H_
#define WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_INGEST_COMMAND_H_

#include <string>
#include <vector>

#include "command_handler/commands/interface/i_command.hpp"

class IngestCommand : public ICommand {
 public:
  bool execute(const std::vector<std::string>& args,
               AppController& controller) override;

 private:
  static auto parse_args(const std::vector<std::string>& args,
                         std::string& path, bool& write_json) -> void;
};

#endif  // WINDOWS_PRESENTATION_CLI_COMMAND_HANDLER_COMMANDS_INGEST_COMMAND_H_
