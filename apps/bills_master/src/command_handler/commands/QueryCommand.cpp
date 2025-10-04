// command_handler/commands/QueryCommand.cpp
#include "QueryCommand.hpp"
#include <stdexcept>

QueryCommand::QueryCommand(std::string format)
    : m_format_str(std::move(format)) {}

bool QueryCommand::execute(const std::vector<std::string>& args, AppController& controller) {
    if (args.size() < 2) {
        throw std::runtime_error("Incomplete query command. Use 'query year <YYYY>' or 'query month <YYYYMM>'.");
    }

    const std::string& query_type = args[0];
    std::vector<std::string> values(args.begin() + 1, args.end());

    if (query_type == "year" || query_type == "y") {
        if (values.empty()) throw std::runtime_error("Missing <year> for 'query year' command.");
        return controller.handle_export("year", values, m_format_str);
    }
    if (query_type == "month" || query_type == "m") {
        if (values.empty()) throw std::runtime_error("Missing <month> for 'query month' command.");
        return controller.handle_export("month", values, m_format_str);
    }

    throw std::runtime_error("Unknown sub-command for 'query': " + query_type);
}