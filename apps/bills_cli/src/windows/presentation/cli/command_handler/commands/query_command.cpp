// command_handler/commands/QueryCommand.cpp
#include "query_command.hpp"

#include <stdexcept>

#include "controllers/app_controller.hpp"

QueryCommand::QueryCommand(std::string format, std::string export_pipeline)
    : m_format_str(std::move(format)),
      m_export_pipeline(std::move(export_pipeline)) {}

auto QueryCommand::execute(const std::vector<std::string>& args,
                           AppController& controller) -> bool {
  if (args.size() < 2) {
    throw std::runtime_error(
        "Incomplete query command. Use 'query year <YYYY>' or 'query month "
        "<YYYY-MM>'.");
  }

  const std::string& query_type = args[0];
  std::vector<std::string> values(args.begin() + 1, args.end());

  if (query_type == "year" || query_type == "y") {
    if (values.empty()) {
      throw std::runtime_error("Missing <year> for 'query year' command.");
    }
    return controller.handle_export("year", values, m_format_str,
                                    m_export_pipeline);
  }
  if (query_type == "month" || query_type == "m") {
    if (values.empty()) {
      throw std::runtime_error("Missing <month> for 'query month' command.");
    }
    return controller.handle_export("month", values, m_format_str,
                                    m_export_pipeline);
  }

  throw std::runtime_error("Unknown sub-command for 'query': " + query_type);
}
