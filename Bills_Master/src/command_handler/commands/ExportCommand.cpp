// command_handler/commands/ExportCommand.cpp
#include "ExportCommand.hpp"
#include <stdexcept>
#include <print>

ExportCommand::ExportCommand(std::string format, std::string type_filter)
    : m_format_str(std::move(format)), m_type_filter(std::move(type_filter)) {}

bool ExportCommand::execute(const std::vector<std::string>& args, AppController& controller) {
    if (args.empty()) {
        throw std::runtime_error("Missing sub-command for 'export'. Use 'all' or 'date'.");
    }

    const std::string& sub_command = args[0];
    std::vector<std::string> values(args.begin() + 1, args.end());

    if (sub_command == "all" || sub_command == "a") {
        return handle_export_all(controller);
    }
    if (sub_command == "date" || sub_command == "d") {
        return handle_export_date(values, controller);
    }

    throw std::runtime_error("Unknown sub-command for 'export': " + sub_command);
}

bool ExportCommand::handle_export_all(AppController& controller) {
    std::vector<std::string> formats_to_process;
    if (m_format_str == "all" || m_format_str == "a") {
        formats_to_process = {"md", "tex", "rst", "typ"};
        std::println("\n--- Batch export for all formats requested ---");
    } else {
        formats_to_process.push_back(m_format_str);
    }

    bool all_successful = true;
    for (const auto& current_format : formats_to_process) {
        std::string export_target = "all";
        if (m_type_filter == "month" || m_type_filter == "m") {
            export_target = "all_months";
        } else if (m_type_filter == "year" || m_type_filter == "y") {
            export_target = "all_years";
        } else if (!m_type_filter.empty()) {
            throw std::runtime_error("Unknown value for --type: '" + m_type_filter + "'. Use 'month'/'m' or 'year'/'y'.");
        }

        if (formats_to_process.size() > 1) {
            std::println("\n-> Processing format: {}", current_format);
        }

        if (!controller.handle_export(export_target, {}, current_format)) {
            all_successful = false;
        }
    }
    return all_successful;
}

bool ExportCommand::handle_export_date(const std::vector<std::string>& values, AppController& controller) {
    if (values.empty()) {
        throw std::runtime_error("Missing date value(s) for 'export date' command.");
    }
    return controller.handle_export("date", values, m_format_str);
}