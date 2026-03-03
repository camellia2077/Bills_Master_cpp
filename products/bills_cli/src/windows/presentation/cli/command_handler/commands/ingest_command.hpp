// command_handler/commands/IngestCommand.hpp
#ifndef INGEST_COMMAND_HPP
#define INGEST_COMMAND_HPP

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

#endif  // INGEST_COMMAND_HPP
