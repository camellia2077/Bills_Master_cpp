// command_handler/commands/QueryCommand.hpp
#ifndef QUERY_COMMAND_HPP
#define QUERY_COMMAND_HPP

#include "command_handler/commands/interface/ICommand.hpp"

class QueryCommand : public ICommand {
 public:
  QueryCommand(std::string format, std::string export_pipeline);
  bool execute(const std::vector<std::string>& args,
               AppController& controller) override;

 private:
  std::string m_format_str;
  std::string m_export_pipeline;
};

#endif  // QUERY_COMMAND_HPP
